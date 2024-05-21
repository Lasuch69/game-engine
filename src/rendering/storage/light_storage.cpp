#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "light_storage.h"

#define CHECK_IF_VALID(owner, id, what)                                                            \
	if (!owner.has(id)) {                                                                          \
		std::cout << "ERROR: " << what << ": " << id << " is not valid resource!" << std::endl;    \
		return;                                                                                    \
	}

ObjectID LightStorage::lightCreate(LightType type) {
	LightRD light;
	light.type = type;

	return _lights.insert(light);
}

void LightStorage::lightSetTransform(ObjectID light, const glm::mat4 &transform) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].transform = transform;
}

void LightStorage::lightSetRange(ObjectID light, float range) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].range = range;
}

void LightStorage::lightSetColor(ObjectID light, const glm::vec3 &color) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].color = color;
}

void LightStorage::lightSetIntensity(ObjectID light, float intensity) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].intensity = intensity;
}

void LightStorage::lightFree(ObjectID light) {
	_lights.free(light);
}

uint32_t LightStorage::getDirectionalLightCount() {
	uint32_t count = 0;
	for (const auto &[_, light] : _lights.map()) {
		if (light.type != LightType::Directional)
			continue;

		count++;
	}

	return count;
}

uint32_t LightStorage::getPointLightCount() {
	uint32_t count = 0;
	for (const auto &[_, light] : _lights.map()) {
		if (light.type != LightType::Point)
			continue;

		count++;
	}

	return count;
}

vk::DescriptorSetLayout LightStorage::getLightSetLayout() const {
	return _lightSetLayout;
}

vk::DescriptorSet LightStorage::getLightSet() const {
	return _lightSet;
}

void LightStorage::initialize(
		vk::Device device, VmaAllocator allocator, vk::DescriptorPool descriptorPool) {
	if (_initialized)
		return;

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {};
	bindings[0].setBinding(0);
	bindings[0].setDescriptorType(vk::DescriptorType::eStorageBuffer);
	bindings[0].setDescriptorCount(1);
	bindings[0].setStageFlags(vk::ShaderStageFlagBits::eFragment);

	bindings[1].setBinding(1);
	bindings[1].setDescriptorType(vk::DescriptorType::eStorageBuffer);
	bindings[1].setDescriptorCount(1);
	bindings[1].setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.setBindings(bindings);

	vk::Result err = device.createDescriptorSetLayout(&createInfo, nullptr, &_lightSetLayout);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Light descriptor set layout creation failed!");

	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.setDescriptorPool(descriptorPool);
	allocInfo.setDescriptorSetCount(1);
	allocInfo.setSetLayouts(_lightSetLayout);

	err = device.allocateDescriptorSets(&allocInfo, &_lightSet);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Light descriptor set allocation failed!");

	vk::BufferUsageFlags usage =
			vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;

	{
		vk::DeviceSize size = sizeof(DirectionalData) * MAX_DIRECTIONAL_LIGHT_COUNT;
		_directionalBuffer =
				AllocatedBuffer::create(allocator, usage, size, &_directionalAllocInfo);
	}

	{
		vk::DeviceSize size = sizeof(PunctualData) * MAX_POINT_LIGHT_COUNT;
		_pointBuffer = AllocatedBuffer::create(allocator, usage, size, &_pointAllocInfo);
	}

	vk::DescriptorBufferInfo directionalLightBufferInfo = _directionalBuffer.getBufferInfo();
	vk::DescriptorBufferInfo pointLightBufferInfo = _pointBuffer.getBufferInfo();

	std::array<vk::WriteDescriptorSet, 2> writeInfos = {};
	writeInfos[0].setDstSet(_lightSet);
	writeInfos[0].setDstBinding(0);
	writeInfos[0].setDstArrayElement(0);
	writeInfos[0].setDescriptorType(vk::DescriptorType::eStorageBuffer);
	writeInfos[0].setDescriptorCount(1);
	writeInfos[0].setBufferInfo(directionalLightBufferInfo);

	writeInfos[1].setDstSet(_lightSet);
	writeInfos[1].setDstBinding(1);
	writeInfos[1].setDstArrayElement(0);
	writeInfos[1].setDescriptorType(vk::DescriptorType::eStorageBuffer);
	writeInfos[1].setDescriptorCount(1);
	writeInfos[1].setBufferInfo(pointLightBufferInfo);

	device.updateDescriptorSets(writeInfos, nullptr);
}

void LightStorage::update() {
	uint32_t directionalLightIndex = 0;
	std::vector<DirectionalData> directionalLightData(MAX_DIRECTIONAL_LIGHT_COUNT);

	uint32_t pointLightIndex = 0;
	std::vector<PunctualData> pointLightData(MAX_POINT_LIGHT_COUNT);

	for (const auto &[_, light] : _lights.map()) {
		if (light.type == LightType::Directional) {
			glm::vec3 direction(0.0, 0.0, -1.0);
			direction = glm::mat3(light.transform) * direction;

			DirectionalData data;
			memcpy(data.direction, &direction, sizeof(data.direction));
			memcpy(data.color, &light.color, sizeof(data.color));
			data.intensity = light.intensity;

			directionalLightData[directionalLightIndex] = data;
			directionalLightIndex++;
			continue;
		}

		if (light.type == LightType::Point) {
			glm::vec3 position(light.transform[3]);

			PunctualData data;
			memcpy(data.position, &position, sizeof(data.position));
			memcpy(data.color, &light.color, sizeof(data.color));
			data.intensity = light.intensity;

			pointLightData[pointLightIndex] = data;
			pointLightIndex++;
			continue;
		}
	}

	{
		void *pDiretionalLightData = _directionalAllocInfo.pMappedData;
		size_t size = sizeof(DirectionalData) * MAX_DIRECTIONAL_LIGHT_COUNT;
		memcpy(pDiretionalLightData, directionalLightData.data(), size);
	}

	{
		void *pPointLightData = _pointAllocInfo.pMappedData;
		size_t size = sizeof(PunctualData) * MAX_POINT_LIGHT_COUNT;
		memcpy(pPointLightData, pointLightData.data(), size);
	}
}
