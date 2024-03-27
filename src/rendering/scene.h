#ifndef SCENE_H
#define SCENE_H

#include <cstdint>
#include <vector>

#include "vertex.h"

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct PointLight {
	glm::vec3 color;

	float intensity;
	float range;
};

#endif // !SCENE_H
