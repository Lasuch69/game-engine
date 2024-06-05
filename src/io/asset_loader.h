#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <rendering/types/vertex.h>

#include "image.h"
#include "mesh.h"

namespace AssetLoader {

enum class LightType {
	Directional,
	Point,
};

struct Material {
	std::optional<uint64_t> albedoIndex;
	std::optional<uint64_t> normalIndex;
	std::optional<uint64_t> metallicIndex;
	std::optional<uint64_t> roughnessIndex;
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

Scene loadGltf(const std::filesystem::path &file);

} // namespace AssetLoader

#endif // !ASSET_LOADER_H
