#ifndef LIGHT_STORAGE_H
#define LIGHT_STORAGE_H

#include <glm/glm.hpp>

#include "../object_owner.h"
#include "../types/allocated.h"

const uint32_t MAX_DIRECTIONAL_LIGHT_COUNT = 8;
const uint32_t MAX_POINT_LIGHT_COUNT = 2048;

class LightStorage {
	struct DirectionalLightRD {
		glm::vec3 direction;
		float intensity;
		glm::vec3 color;
		float _padding;
	};
	static_assert(sizeof(DirectionalLightRD) % 16 == 0, "DirectionalLightRD is not multiple of 16");

	struct PointLightRD {
		glm::vec3 position;
		float range;
		glm::vec3 color;
		float intensity;
	};
	static_assert(sizeof(PointLightRD) % 16 == 0, "PointLightRD is not multiple of 16");

	AllocatedBuffer _directionalLightBuffer;
	VmaAllocationInfo _directionalLightAllocInfo;

	AllocatedBuffer _pointLightBuffer;
	VmaAllocationInfo _pointLightAllocInfo;

	ObjectOwner<DirectionalLightRD> _directionalLights;
	ObjectOwner<PointLightRD> _pointLights;

	vk::DescriptorSetLayout _lightSetLayout;
	vk::DescriptorSet _lightSet;

	bool _initialized = false;

public:
	ObjectID directionalLightCreate();
	void directionalLightSetDirection(ObjectID directionalLight, const glm::vec3 &direction);
	void directionalLightSetIntensity(ObjectID directionalLight, float intensity);
	void directionalLightSetColor(ObjectID directionalLight, const glm::vec3 &color);
	void directionalLightFree(ObjectID directionalLight);

	ObjectID pointLightCreate();
	void pointLightSetPosition(ObjectID pointLight, const glm::vec3 &position);
	void pointLightSetRange(ObjectID pointLight, float range);
	void pointLightSetColor(ObjectID pointLight, const glm::vec3 &color);
	void pointLightSetIntensity(ObjectID pointLight, float intensity);
	void pointLightFree(ObjectID pointLight);

	uint32_t getDirectionalLightCount() const;
	uint32_t getPointLightCount() const;

	vk::DescriptorSetLayout getLightSetLayout() const;
	vk::DescriptorSet getLightSet() const;

	void initialize(vk::Device device, VmaAllocator allocator, vk::DescriptorPool descriptorPool);
	void update();
};

#endif // !LIGHT_STORAGE_H
