#ifndef LOADER_H
#define LOADER_H

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "rendering/types/vertex.h"

namespace Loader {

struct Camera {
	float fovY;
	float zNear;
	float zFar;
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct PointLight {
	glm::vec3 color;
	float intensity;
	float range;
};

struct Node {
	std::string name;
	glm::mat4 transform;

	std::optional<Camera> camera;
	std::optional<Mesh> mesh;
	std::optional<PointLight> pointLight;
};

std::vector<Node> loadScene(std::filesystem::path path);

} // namespace Loader

#endif // !LOADER_H
