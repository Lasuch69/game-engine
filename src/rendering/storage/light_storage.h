#ifndef LIGHT_STORAGE_H
#define LIGHT_STORAGE_H

#include <glm/glm.hpp>

#include "../../rid_owner.h"
#include "../types/allocated.h"

const uint32_t MAX_DIRECTIONAL_LIGHT_COUNT = 8;
const uint32_t MAX_POINT_LIGHT_COUNT = 2048;

typedef uint64_t DirectionalLight;
typedef uint64_t PointLight;

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

	RIDOwner<DirectionalLightRD> _directionalLights;
	RIDOwner<PointLightRD> _pointLights;

	vk::DescriptorSetLayout _lightSetLayout;
	vk::DescriptorSet _lightSet;

	bool _initialized = false;

public:
	DirectionalLight directionalLightCreate();
	void directionalLightSetDirection(
			DirectionalLight directionalLight, const glm::vec3 &direction);
	void directionalLightSetIntensity(DirectionalLight directionalLight, float intensity);
	void directionalLightSetColor(DirectionalLight directionalLight, const glm::vec3 &color);
	void directionalLightFree(DirectionalLight directionalLight);

	PointLight pointLightCreate();
	void pointLightSetPosition(PointLight pointLight, const glm::vec3 &position);
	void pointLightSetRange(PointLight pointLight, float range);
	void pointLightSetColor(PointLight pointLight, const glm::vec3 &color);
	void pointLightSetIntensity(PointLight pointLight, float intensity);
	void pointLightFree(PointLight pointLight);

	uint32_t getDirectionalLightCount() const;
	uint32_t getPointLightCount() const;

	vk::DescriptorSetLayout getLightSetLayout() const;
	vk::DescriptorSet getLightSet() const;

	void initialize(vk::Device device, VmaAllocator allocator, vk::DescriptorPool descriptorPool);
	void update();
};

#endif // !LIGHT_STORAGE_H
