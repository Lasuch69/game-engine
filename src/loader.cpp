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

glm::mat4 _getTransformMatrix(const fastgltf::Node &node) {
	/** Both a matrix and TRS values are not allowed
	 * to exist at the same time according to the spec */
	if (const auto *pMatrix = std::get_if<fastgltf::Node::TransformMatrix>(&node.transform)) {
		return glm::mat4x4(glm::make_mat4x4(pMatrix->data()));
	}

	if (const auto *pTransform = std::get_if<fastgltf::TRS>(&node.transform)) {
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

Image::Format _channelsToFormat(int channels) {
	switch (channels) {
		case 1:
			return Image::Format::L8;
		case 2:
			return Image::Format::LA8;
		case 3:
			return Image::Format::RGB8;
		case 4:
			return Image::Format::RGBA8;
		default:
			return Image::Format::L8;
	}
}

std::shared_ptr<Image> _createImage(
		int width, int height, int channels, const std::vector<uint8_t> &data) {
	if (channels == 4)
		return Image::create(width, height, _channelsToFormat(channels), data);

	size_t pixelCount = width * height;
	std::vector<uint8_t> newData(pixelCount * 4);

	for (size_t pixel = 0; pixel < pixelCount; pixel++) {
		size_t oldIdx = pixel * channels;
		size_t newIdx = pixel * 4;

		switch (channels) {
			case 1:
				newData[newIdx + 0] = data[oldIdx + 0];
				newData[newIdx + 1] = 0;
				newData[newIdx + 2] = 0;
				newData[newIdx + 3] = 255;
				break;
			case 2:
				newData[newIdx + 0] = data[oldIdx + 0];
				newData[newIdx + 1] = 0;
				newData[newIdx + 2] = 0;
				newData[newIdx + 3] = data[oldIdx + 1];
				break;
			case 3:
				newData[newIdx + 0] = data[oldIdx + 0];
				newData[newIdx + 1] = data[oldIdx + 1];
				newData[newIdx + 2] = data[oldIdx + 2];
				newData[newIdx + 3] = 255;
				break;
			default:
				return nullptr;
		}
	}

	return Image::create(width, height, Image::Format::RGBA8, newData);
}

std::shared_ptr<Image> _loadImage(
		fastgltf::Image &image, fastgltf::Asset *pAsset, const std::filesystem::path &basePath) {
	std::shared_ptr<Image> pImage = nullptr;

	std::visit(
			fastgltf::visitor{
					[](auto &arg) {},
					[&](fastgltf::sources::URI &filePath) {
						assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
						assert(filePath.uri.isLocalPath());	  // We're only capable of loading local
															  // files.

						const std::filesystem::path path(filePath.uri.path().begin(),
								filePath.uri.path().end()); // Thanks C++.

						int width, height, channels = 0;
						stbi_uc *pData = stbi_load((basePath / path).c_str(), &width, &height,
								&channels, STBI_default);

						if (pData == nullptr)
							return;

						size_t size = width * height * channels;

						std::vector<uint8_t> data(size);
						memcpy(data.data(), pData, size);
						stbi_image_free(pData);

						pImage = _createImage(width, height, channels, data);
					},
					[&](fastgltf::sources::Vector &vector) {
						int width, height, channels = 0;
						unsigned char *pData = stbi_load_from_memory(vector.bytes.data(),
								static_cast<int>(vector.bytes.size()), &width, &height, &channels,
								STBI_default);

						if (pData == nullptr)
							return;

						size_t size = width * height * channels;

						std::vector<uint8_t> data(size);
						memcpy(data.data(), pData, size);
						stbi_image_free(pData);

						pImage = _createImage(width, height, channels, data);
					},
					[&](fastgltf::sources::BufferView &view) {
						auto &bufferView = pAsset->bufferViews[view.bufferViewIndex];
						auto &buffer = pAsset->buffers[bufferView.bufferIndex];
						std::visit(fastgltf::visitor{
										   // We only care about VectorWithMime here, because we
										   // specify LoadExternalBuffers, meaning
										   // all buffers are already loaded into a vector.
										   [](auto &arg) {},
										   [&](fastgltf::sources::Vector &vector) {
											   int width, height, channels = 0;
											   unsigned char *pData = stbi_load_from_memory(
													   vector.bytes.data(),
													   static_cast<int>(vector.bytes.size()),
													   &width, &height, &channels, STBI_default);

											   if (pData == nullptr)
												   return;

											   size_t size = width * height * channels;

											   std::vector<uint8_t> data(size);
											   memcpy(data.data(), pData, size);
											   stbi_image_free(pData);

											   pImage = _createImage(width, height, channels, data);
										   } },
								buffer.data);
					},
			},
			image.data);

	return pImage;
}

Mesh _loadMesh(fastgltf::Asset *pAsset, const fastgltf::Mesh &mesh) {
	std::vector<Primitive> primitives = {};

	for (const fastgltf::Primitive &primitive : mesh.primitives) {
		std::vector<Vertex> vertices;

		auto *positionIter = primitive.findAttribute("POSITION");
		auto &positionAccessor = pAsset->accessors[positionIter->second];

		// positions are required
		if (!positionAccessor.bufferViewIndex.has_value())
			continue;

		vertices.resize(positionAccessor.count);

		fastgltf::iterateAccessorWithIndex<glm::vec3>(
				*pAsset, positionAccessor, [&](glm::vec3 position, size_t idx) {
					vertices[idx].position = position;
					vertices[idx].color = glm::vec3(1.0f); // default value
				});

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

		std::optional<size_t> materialIndex = {};
		if (primitive.materialIndex.has_value())
			materialIndex = primitive.materialIndex.value();

		primitives.push_back(Primitive{
				vertices,
				indices,
				materialIndex,
		});
	}

	return Mesh{ mesh.name.c_str(), primitives };
}

Gltf *Loader::loadGltf(std::filesystem::path path) {
	fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);

	fastgltf::GltfDataBuffer data;
	data.loadFromFile(path);

	fastgltf::Expected<fastgltf::Asset> result = parser.loadGltf(&data, path.parent_path(),
			fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices);

	if (fastgltf::Error err = result.error(); err != fastgltf::Error::None) {
		std::cout << "Failed to load GlTF: " << fastgltf::getErrorMessage(err) << std::endl;
		return nullptr;
	}

	fastgltf::Asset *pAsset = result.get_if();

	Gltf *pGltf = new Gltf;
	pGltf->name = path.stem();

	for (const fastgltf::Material &material : pAsset->materials) {
		Material materialData = {};

		const std::optional<fastgltf::TextureInfo> &albedoInfo = material.pbrData.baseColorTexture;
		if (albedoInfo.has_value()) {
			materialData.albedoIndex = albedoInfo->textureIndex;
		}

		pGltf->materials.push_back(materialData);
	}

	for (fastgltf::Image &image : pAsset->images) {
		std::shared_ptr<Image> pImage = _loadImage(image, pAsset, path.parent_path());

		if (pImage == nullptr) {
			std::cout << "Failed to load image: " << image.name << std::endl;
		}

		pGltf->images.push_back(pImage);
	}

	for (const fastgltf::Mesh &mesh : pAsset->meshes) {
		pGltf->meshes.push_back(_loadMesh(pAsset, mesh));
	}

	for (const fastgltf::Node &node : pAsset->nodes) {
		std::string name = node.name.c_str();
		glm::mat4 transform = _getTransformMatrix(node);

		if (node.cameraIndex.has_value()) {
			// WHY?!?!?
			std::visit(fastgltf::visitor{
							   [&](fastgltf::Camera::Perspective &perspective) {
								   pGltf->cameras.push_back(Camera{
										   name,
										   transform,
										   perspective.yfov,
										   perspective.znear,
										   perspective.zfar.value_or(100.0f),
								   });
							   },
							   [&](fastgltf::Camera::Orthographic &orthographic) {},
					   },
					pAsset->cameras[node.cameraIndex.value()].camera);
		}

		if (node.meshIndex.has_value()) {
			pGltf->meshInstances.push_back(MeshInstance{
					name,
					transform,
					node.meshIndex.value(),
			});
		}

		if (node.lightIndex.has_value()) {
			fastgltf::Light light = pAsset->lights[node.lightIndex.value()];

			glm::vec3 color = glm::make_vec3(light.color.data());

			// 200 lumen = 1
			float intensity = (light.intensity * STERADIAN) / 200.0f;

			switch (light.type) {
				case fastgltf::LightType::Point:
					pGltf->pointLights.push_back(PointLight{
							name,
							glm::vec3(transform[3]),
							color,
							intensity,
							light.range.value_or(0),
					});
				case fastgltf::LightType::Spot:
				case fastgltf::LightType::Directional:
					break;
			}
		}
	}

	return pGltf;
}
