#ifndef LOADER_H
#define LOADER_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "rendering/types/vertex.h"

#include "image.h"

namespace Loader {

struct Camera {
	std::string name;
	glm::mat4 transform;
	float fovY;
	float zNear;
	float zFar;
};

struct Primitive {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	size_t materialIndex;
};

struct Mesh {
	std::string name;
	std::vector<Primitive> primitives;
};

struct MeshInstance {
	std::string name;
	glm::mat4 transform;
	size_t meshIndex;
};

struct PointLight {
	std::string name;
	glm::vec3 position;
	glm::vec3 color;
	float intensity;
	float range;
};

struct Material {
	size_t albedoIndex;
};

struct SceneGlTF {
	std::string name;
	glm::mat4 transform;

	std::vector<Material> materials;
	std::vector<Image *> images;

	std::vector<Mesh> meshes;

	std::vector<Camera> cameras;
	std::vector<MeshInstance> meshInstances;
	std::vector<PointLight> pointLights;
};

SceneGlTF *loadGltf(std::filesystem::path path);

} // namespace Loader

#endif // !LOADER_H
