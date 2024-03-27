#ifndef LOADER_H
#define LOADER_H

#include <filesystem>
#include <optional>
#include <string>

#include <glm/glm.hpp>

#include "fastgltf/types.hpp"
#include "rendering/scene.h"

const float STERADIAN = glm::pi<float>() * 4.0f;

struct Node {
	std::string name;
	glm::mat4 transform;

	std::optional<fastgltf::Camera::Perspective> camera;
	std::optional<Mesh> mesh;
	std::optional<PointLight> pointLight;
};

class Loader {
public:
	static std::vector<Node> loadScene(std::filesystem::path path);
};

#endif // !LOADER_H
