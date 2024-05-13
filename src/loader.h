#ifndef LOADER_H
#define LOADER_H

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "image.h"
#include "rendering/types/vertex.h"

namespace Loader {

enum class LightType {
	Directional,
	Point,
};

struct Primitive {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	uint64_t materialIndex;
};

struct Material {
	std::optional<uint64_t> albedoIndex;
	std::optional<uint64_t> normalIndex;
	std::optional<uint64_t> roughnessIndex;
	std::string name;
};

struct Mesh {
	std::vector<Primitive> primitives;
	std::string name;
};

struct MeshInstance {
	glm::mat4 transform;
	uint64_t meshIndex;
	std::string name;
};

struct Light {
	glm::mat4 transform;
	LightType type;

	glm::vec3 color;
	float intensity;

	std::optional<float> range;
	std::string name;
};

struct Scene {
	std::vector<std::shared_ptr<Image>> images;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<MeshInstance> meshInstances;
	std::vector<Light> lights;
};

std::optional<Scene> loadGltf(const std::filesystem::path &path);

} // namespace Loader

#endif // !LOADER_H
