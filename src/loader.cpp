#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/types.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <stb/stb_image.h>

#include "loader.h"

// needed for candela to lumen conversion
const float STERADIAN = glm::pi<float>() * 4.0f;

using namespace Loader;

glm::mat4 _getTransformMatrix(const fastgltf::Node &gltfNode) {
	/** Both a matrix and TRS values are not allowed
	 * to exist at the same time according to the spec */
	if (const auto *pMatrix = std::get_if<fastgltf::Node::TransformMatrix>(&gltfNode.transform)) {
		return glm::mat4x4(glm::make_mat4x4(pMatrix->data()));
	}

	if (const auto *pTransform = std::get_if<fastgltf::TRS>(&gltfNode.transform)) {
		// Warning: The quaternion to mat4x4 conversion here is not correct with all versions of
		// glm. glTF provides the quaternion as (x, y, z, w), which is the same layout glm used up
		// to version 0.9.9.8. However, with commit 59ddeb7 (May 2021) the default order was changed
		// to (w, x, y, z). You could either define GLM_FORCE_QUAT_DATA_XYZW to return to the old
		// layout, or you could use the recently added static factory constructor glm::quat::wxyz(w,
		// x, y, z), which guarantees the parameter order.
		return glm::translate(glm::mat4(1.0f), glm::make_vec3(pTransform->translation.data())) *
			   glm::toMat4(glm::make_quat(pTransform->rotation.data())) *
			   glm::scale(glm::mat4(1.0f), glm::make_vec3(pTransform->scale.data()));
	}

	return glm::mat4(1.0f);
}

Image *_loadImage(fastgltf::Asset *pAsset, fastgltf::Image &gltfImage,
		const std::filesystem::path &rootPath) {
	int width, height, channels = 0;
	stbi_uc *pData = nullptr;

	auto loadFromPath = [&](fastgltf::sources::URI &filepath) {
		assert(filepath.fileByteOffset == 0); // We don't support offsets with stbi.

		std::filesystem::path path(filepath.uri.path().begin(),
				filepath.uri.path().end()); // Thanks C++.

		// should be absolute
		path = (rootPath / path);
		assert(path.is_absolute());

		pData = stbi_load(path.c_str(), &width, &height, &channels, STBI_default);
	};

	auto loadFromMemory = [&](fastgltf::sources::Vector &vector) {
		uint8_t *pBytes = vector.bytes.data();
		int len = static_cast<int>(vector.bytes.size());

		pData = stbi_load_from_memory(pBytes, len, &width, &height, &channels, STBI_default);
	};

	fastgltf::visitor visitor = fastgltf::visitor{
		[](auto &arg) {},
		[&](fastgltf::sources::URI &filepath) { loadFromPath(filepath); },
		[&](fastgltf::sources::Vector &vector) { loadFromMemory(vector); },
		[&](fastgltf::sources::BufferView &view) {
			auto &bufferView = pAsset->bufferViews[view.bufferViewIndex];
			auto &buffer = pAsset->buffers[bufferView.bufferIndex];

			// We only care about VectorWithMime here, because we
			// specify LoadExternalBuffers, meaning
			// all buffers are already loaded into a vector.
			fastgltf::visitor visitor = fastgltf::visitor{
				[](auto &arg) {},
				[&](fastgltf::sources::Vector &vector) { loadFromMemory(vector); },
			};

			std::visit(visitor, buffer.data);
		},
	};

	std::visit(visitor, gltfImage.data);

	if (pData == nullptr)
		return nullptr;

	size_t size = width * height * channels;

	std::vector<uint8_t> data(size);
	memcpy(data.data(), pData, size);
	stbi_image_free(pData);

	return new Image(width, height, channels, data);
}

void _generateTangents(const std::vector<uint32_t> &indices, std::vector<Vertex> &vertices) {
	assert(indices.size() % 3 == 0);

	std::vector<float> avg(vertices.size());

	for (size_t i = 0; i < indices.size(); i += 3) {
		Vertex &v0 = vertices[indices[i + 0]];
		Vertex &v1 = vertices[indices[i + 1]];
		Vertex &v2 = vertices[indices[i + 2]];

		glm::vec3 pos0 = v0.position;
		glm::vec3 pos1 = v1.position;
		glm::vec3 pos2 = v2.position;

		glm::vec2 uv0 = v0.texCoord;
		glm::vec2 uv1 = v1.texCoord;
		glm::vec2 uv2 = v2.texCoord;

		glm::vec3 deltaPos1 = pos1 - pos0;
		glm::vec3 deltaPos2 = pos2 - pos0;

		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;

		v0.tangent += tangent;
		v1.tangent += tangent;
		v2.tangent += tangent;

		avg[indices[i + 0]] += 1.0;
		avg[indices[i + 1]] += 1.0;
		avg[indices[i + 2]] += 1.0;
	}

	for (size_t i = 0; i < avg.size(); i++) {
		float denom = 1.0 / avg[i];
		vertices[i].tangent *= denom;
	}
}

Mesh _loadMesh(fastgltf::Asset *pAsset, const fastgltf::Mesh &gltfMesh) {
	std::vector<Primitive> primitives = {};

	for (const fastgltf::Primitive &primitive : gltfMesh.primitives) {
		std::vector<Vertex> vertices;

		auto *positionIter = primitive.findAttribute("POSITION");
		auto &positionAccessor = pAsset->accessors[positionIter->second];

		// positions are required
		if (!positionAccessor.bufferViewIndex.has_value())
			continue;

		vertices.resize(positionAccessor.count);

		fastgltf::iterateAccessorWithIndex<glm::vec3>(*pAsset, positionAccessor,
				[&](glm::vec3 position, size_t idx) { vertices[idx].position = position; });

		auto *normalIter = primitive.findAttribute("NORMAL");
		auto &normalAccessor = pAsset->accessors[normalIter->second];
		if (normalAccessor.bufferViewIndex.has_value()) {
			fastgltf::iterateAccessorWithIndex<glm::vec3>(*pAsset, normalAccessor,
					[&](glm::vec3 normal, size_t idx) { vertices[idx].normal = normal; });
		}

		auto *texCoordIter = primitive.findAttribute("TEXCOORD_0");
		auto &texCoordAccessor = pAsset->accessors[texCoordIter->second];
		if (texCoordAccessor.bufferViewIndex.has_value()) {
			fastgltf::iterateAccessorWithIndex<glm::vec2>(*pAsset, texCoordAccessor,
					[&](glm::vec2 texCoord, size_t idx) { vertices[idx].texCoord = texCoord; });
		}

		std::vector<uint32_t> indices = {};

		std::size_t idx = 0;

		auto &indexAccessor = pAsset->accessors[primitive.indicesAccessor.value()];
		indices.resize(indexAccessor.count);
		fastgltf::iterateAccessor<std::uint32_t>(
				*pAsset, indexAccessor, [&](std::uint32_t index) { indices[idx++] = index; });

		_generateTangents(indices, vertices);

		size_t materialIndex = primitive.materialIndex.value_or(0);

		primitives.push_back(Primitive{
				vertices,
				indices,
				materialIndex,
		});
	}

	return Mesh{
		primitives,
		gltfMesh.name.c_str(),
	};
}

std::optional<Scene> Loader::loadGltf(const std::filesystem::path &path) {
	fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);

	fastgltf::GltfDataBuffer data;
	data.loadFromFile(path);

	fastgltf::Expected<fastgltf::Asset> result = parser.loadGltf(&data, path.parent_path(),
			fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices);

	if (fastgltf::Error err = result.error(); err != fastgltf::Error::None) {
		std::cout << "Failed to load GlTF: " << fastgltf::getErrorMessage(err) << std::endl;
		return {};
	}

	fastgltf::Asset *pAsset = result.get_if();

	std::vector<Image *> images;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<MeshInstance> meshInstances;
	std::vector<Light> lights;

	for (const fastgltf::Material &gltfMaterial : pAsset->materials) {
		Material material = {};

		const std::optional<fastgltf::TextureInfo> &albedoInfo =
				gltfMaterial.pbrData.baseColorTexture;

		if (albedoInfo.has_value()) {
			fastgltf::Image &gltfImage = pAsset->images[albedoInfo->textureIndex];
			Image *pImage = _loadImage(pAsset, gltfImage, path.parent_path());

			uint32_t idx = images.size();
			images.push_back(pImage);

			material.albedoIndex = idx;
		}

		const std::optional<fastgltf::NormalTextureInfo> &normalInfo = gltfMaterial.normalTexture;

		if (normalInfo.has_value()) {
			fastgltf::Image &gltfImage = pAsset->images[normalInfo->textureIndex];
			Image *pImage = _loadImage(pAsset, gltfImage, path.parent_path());

			uint32_t idx = images.size();
			images.push_back(pImage);

			material.normalIndex = idx;
		}

		const std::optional<fastgltf::TextureInfo> &roughnessInfo =
				gltfMaterial.pbrData.metallicRoughnessTexture;

		if (roughnessInfo.has_value()) {
			fastgltf::Image &gltfImage = pAsset->images[roughnessInfo->textureIndex];
			Image *pImage = _loadImage(pAsset, gltfImage, path.parent_path());

			uint32_t idx = images.size();
			images.push_back(pImage);

			material.roughnessIndex = idx;
		}

		materials.push_back(material);
	}

	for (const fastgltf::Mesh &gltfMesh : pAsset->meshes) {
		Mesh mesh = _loadMesh(pAsset, gltfMesh);
		meshes.push_back(mesh);
	}

	for (const fastgltf::Node &gltfNode : pAsset->nodes) {
		glm::mat4 transform = _getTransformMatrix(gltfNode);
		std::string name = gltfNode.name.c_str();

		const fastgltf::Optional<size_t> &meshIndex = gltfNode.meshIndex;
		if (meshIndex.has_value()) {
			MeshInstance meshInstance = {
				transform,
				gltfNode.meshIndex.value(),
				name,
			};

			meshInstances.push_back(meshInstance);
		}

		const fastgltf::Optional<size_t> &lightIndex = gltfNode.lightIndex;
		if (lightIndex.has_value()) {
			fastgltf::Light gltfLight = pAsset->lights[lightIndex.value()];

			LightType lightType = LightType::Point;
			glm::vec3 color = glm::make_vec3(gltfLight.color.data());
			float intensity = gltfLight.intensity;

			std::optional<float> range = {};

			if (gltfLight.type == fastgltf::LightType::Point) {
				if (gltfLight.range.has_value())
					range = gltfLight.range.value();

				intensity *= STERADIAN / 1000.0f;
				lightType = LightType::Point;
			} else if (gltfLight.type == fastgltf::LightType::Directional) {
				lightType = LightType::Directional;
			} else {
				continue;
			}

			Light light = {
				transform,
				lightType,
				color,
				intensity,
				range,
				name,
			};

			lights.push_back(light);
		}
	}

	return Scene{
		images,
		materials,
		meshes,
		meshInstances,
		lights,
	};
}
