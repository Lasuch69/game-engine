#ifndef LIGHT_STORAGE_H
#define LIGHT_STORAGE_H

#include <glm/glm.hpp>

#include "../object_owner.h"
#include "../types/allocated.h"

const uint32_t MAX_DIRECTIONAL_LIGHT_COUNT = 8;
const uint32_t MAX_POINT_LIGHT_COUNT = 2048;

enum class LightType {
	Directional,
	Point,
};

class LightStorage {
	struct DirectionalData {
		float direction[3];
		float _padding;

		float color[3];
		float intensity;
	};
	static_assert(sizeof(DirectionalData) % 16 == 0, "DirectionalData is not multiple of 16");

	struct PunctualData {
		float position[3];
		float range;

		float color[3];
		float intensity;
	};
	static_assert(sizeof(PunctualData) % 16 == 0, "PunctualData is not multiple of 16");

	struct LightRD {
		LightType type;

		glm::mat4 transform;
		float range;

		glm::vec3 color;
		float intensity;
	};

	ObjectOwner<LightRD> _lights;

	AllocatedBuffer _directionalBuffer;
	VmaAllocationInfo _directionalAllocInfo;

	AllocatedBuffer _pointBuffer;
	VmaAllocationInfo _pointAllocInfo;

	vk::DescriptorSetLayout _lightSetLayout;
	vk::DescriptorSet _lightSet;

	bool _initialized = false;

public:
	ObjectID lightCreate(LightType type);
	void lightSetTransform(ObjectID light, const glm::mat4 &transform);
	void lightSetRange(ObjectID light, float range);
	void lightSetColor(ObjectID light, const glm::vec3 &color);
	void lightSetIntensity(ObjectID light, float intensity);
	void lightFree(ObjectID light);

	uint32_t getDirectionalLightCount();
	uint32_t getPointLightCount();

	vk::DescriptorSetLayout getLightSetLayout() const;
	vk::DescriptorSet getLightSet() const;

	void initialize(vk::Device device, VmaAllocator allocator, vk::DescriptorPool descriptorPool);
	void update();
};

#endif // !LIGHT_STORAGE_H
