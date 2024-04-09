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

#include "loader.h"

// needed for candela to lumen conversion
const float STERADIAN = glm::pi<float>() * 4.0f;

using namespace Loader;

glm::mat4 getTransformMatrix(const fastgltf::Node &node) {
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

Mesh loadMesh(fastgltf::Asset *pAsset, const fastgltf::Mesh &mesh) {
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
			fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers |
					fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices);

	if (fastgltf::Error err = result.error(); err != fastgltf::Error::None) {
		std::cout << "Failed to load GlTF: " << fastgltf::getErrorMessage(err) << std::endl;
		return nullptr;
	}

	fastgltf::Asset *pAsset = result.get_if();

	Gltf *pGltf = new Gltf;
	pGltf->name = path.stem();

	for (const fastgltf::Mesh &mesh : pAsset->meshes) {
		pGltf->meshes.push_back(loadMesh(pAsset, mesh));
	}

	for (const fastgltf::Node &node : pAsset->nodes) {
		std::string name = node.name.c_str();
		glm::mat4 transform = getTransformMatrix(node);

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

			// 1000 lumen = 1
			float intensity = (light.intensity * STERADIAN) / 1000.0f;

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
