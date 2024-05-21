#ifndef RESOURCE_H
#define RESOURCE_H

#include <cstdint>
#include <glm/glm.hpp>

#include "allocated.h"

typedef uint64_t ObjectID;

struct PrimitiveRD {
	uint32_t indexCount;
	uint32_t firstIndex;
	ObjectID material;
};

struct MeshRD {
	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;
	std::vector<PrimitiveRD> primitives;
};

struct MeshInstanceRD {
	glm::mat4 transform;
	ObjectID mesh;
};

struct MaterialRD {
	vk::DescriptorSet textureSet;
};

struct TextureRD {
	AllocatedImage image;
	vk::ImageView imageView;
	vk::Sampler sampler;
};

#endif // !RESOURCE_H
