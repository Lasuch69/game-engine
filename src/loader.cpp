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

const float STERADIAN = glm::pi<float>() * 4.0f;

glm::mat4 getTransformMatrix(const fastgltf::Node &node, glm::mat4x4 &base) {
	/** Both a matrix and TRS values are not allowed
	 * to exist at the same time according to the spec */
	if (const auto *pMatrix = std::get_if<fastgltf::Node::TransformMatrix>(&node.transform)) {
		return base * glm::mat4x4(glm::make_mat4x4(pMatrix->data()));
	}

	if (const auto *pTransform = std::get_if<fastgltf::TRS>(&node.transform)) {
		// Warning: The quaternion to mat4x4 conversion here is not correct with all versions of
		// glm. glTF provides the quaternion as (x, y, z, w), which is the same layout glm used up
		// to version 0.9.9.8. However, with commit 59ddeb7 (May 2021) the default order was changed
		// to (w, x, y, z). You could either define GLM_FORCE_QUAT_DATA_XYZW to return to the old
		// layout, or you could use the recently added static factory constructor glm::quat::wxyz(w,
		// x, y, z), which guarantees the parameter order.
		return base *
				glm::translate(glm::mat4(1.0f), glm::make_vec3(pTransform->translation.data())) *
				glm::toMat4(glm::make_quat(pTransform->rotation.data())) *
				glm::scale(glm::mat4(1.0f), glm::make_vec3(pTransform->scale.data()));
	}

	return base;
}

std::vector<Loader::Node> Loader::loadScene(std::filesystem::path path) {
	fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);

	fastgltf::GltfDataBuffer data;
	data.loadFromFile(path);

	fastgltf::Expected<fastgltf::Asset> result =
			parser.loadGltf(&data, path.parent_path(), fastgltf::Options::None);

	if (result.error() != fastgltf::Error::None) {
		printf("Failed to load: %s, Error: %d\n", path.c_str(), (int)result.error());
		return std::vector<Loader::Node>();
	}

	fastgltf::Asset *pAsset = result.get_if();

	glm::mat4 base = glm::mat4(1.0f);

	std::vector<Loader::Node> nodes;
	for (const fastgltf::Node &gltfNode : pAsset->nodes) {
		Node node{};
		node.name = gltfNode.name.c_str();
		node.transform = getTransformMatrix(gltfNode, base);

		bool hasCamera = gltfNode.cameraIndex.has_value();
		if (hasCamera) {
			size_t cameraIndex = gltfNode.cameraIndex.value();
			fastgltf::Camera camera = pAsset->cameras[cameraIndex];

			// c++'s "features", WHY?!?!?
			std::visit(fastgltf::visitor{ [&](fastgltf::Camera::Perspective &perspective) {
											 node.camera =
													 Camera{ perspective.yfov, perspective.znear,
														 perspective.zfar.value_or(100.0f) };
										 },
							   [&](fastgltf::Camera::Orthographic &orthographic) {} },
					camera.camera);
		}

		bool hasMesh = gltfNode.meshIndex.has_value();
		if (hasMesh) {
			size_t meshIndex = gltfNode.meshIndex.value();
			fastgltf::Mesh gltfMesh = pAsset->meshes[meshIndex];

			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			for (const fastgltf::Primitive &primitive : gltfMesh.primitives) {
				bool hasIndices = primitive.indicesAccessor.has_value();
				if (hasIndices) {
					auto &accessor = pAsset->accessors[primitive.indicesAccessor.value()];
					indices.resize(accessor.count);

					std::size_t idx = 0;
					fastgltf::iterateAccessor<std::uint32_t>(*pAsset, accessor,
							[&](std::uint32_t index) { indices[idx++] = index; });
				}

				auto *positionIter = primitive.findAttribute("POSITION");
				auto &positionAccessor = pAsset->accessors[positionIter->second];

				bool hasPositions = positionAccessor.bufferViewIndex.has_value();
				if (hasPositions) {
					vertices.resize(positionAccessor.count);

					fastgltf::iterateAccessorWithIndex<glm::vec3>(
							*pAsset, positionAccessor, [&](glm::vec3 position, std::size_t idx) {
								vertices[idx].position = position;

								// default color = (1.0, 1.0, 1.0)
								vertices[idx].color = glm::vec3(1.0f);
							});
				} else {
					continue;
				}

				auto *normalIter = primitive.findAttribute("NORMAL");
				auto &normalAccessor = pAsset->accessors[normalIter->second];

				bool hasNormals = normalAccessor.bufferViewIndex.has_value();
				if (hasNormals) {
					fastgltf::iterateAccessorWithIndex<glm::vec3>(
							*pAsset, normalAccessor, [&](glm::vec3 normal, std::size_t idx) {
								vertices[idx].normal = normal;
							});
				}
			}

			node.mesh = Mesh{ vertices, indices };
		}

		bool hasLight = gltfNode.lightIndex.has_value();
		if (hasLight) {
			size_t lightIndex = gltfNode.lightIndex.value();
			fastgltf::Light gltfLight = pAsset->lights[lightIndex];

			glm::vec3 color = glm::make_vec3(gltfLight.color.data());

			float intensity = gltfLight.intensity;
			intensity *= STERADIAN; // candela to lumen
			intensity /= 1000; // lumen to arbitrary

			if (gltfLight.type == fastgltf::LightType::Point) {
				float range = gltfLight.range.value_or(0.0f);
				node.pointLight = PointLight{ color, intensity, range };
			} else {
				printf("Light of node: %s, has unsupported type.", node.name.c_str());
			}
		}

		nodes.push_back(node);
	}

	return nodes;
}
