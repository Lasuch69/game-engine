#ifndef LIGHT_STORAGE_H
#define LIGHT_STORAGE_H

#include <glm/glm.hpp>

#include "../../rid_owner.h"
#include "../types/allocated.h"

const uint32_t MAX_LIGHT_COUNT = 2048;

typedef uint64_t PointLight;

class LightStorage {
	struct PointLightRD {
		glm::vec3 position;
		float range;
		glm::vec3 color;
		float intensity;
	};

	AllocatedBuffer _pointlightBuffer;
	VmaAllocationInfo _pointlightAllocInfo;

	RIDOwner<PointLightRD> _pointLights;

	vk::DescriptorSetLayout _lightSetLayout;
	vk::DescriptorSet _lightSet;

	bool _initialized = false;

public:
	PointLight pointLightCreate();
	void pointLightSetPosition(PointLight pointLight, const glm::vec3 &position);
	void pointLightSetRange(PointLight pointLight, float range);
	void pointLightSetColor(PointLight pointLight, const glm::vec3 &color);
	void pointLightSetIntensity(PointLight pointLight, float intensity);
	void pointLightFree(PointLight pointLight);

	uint32_t getLightCount() const;

	vk::DescriptorSetLayout getLightSetLayout() const;
	vk::DescriptorSet getLightSet() const;

	void initialize(vk::Device device, VmaAllocator allocator, vk::DescriptorPool descriptorPool);
	void update();
};

#endif // !LIGHT_STORAGE_H
