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

PointLight LightStorage::pointLightCreate() {
	return _pointLights.insert({});
}

void LightStorage::pointLightSetPosition(PointLight pointLight, const glm::vec3 &position) {
	CHECK_IF_VALID(_pointLights, pointLight, "PointLight");
	_pointLights[pointLight].position = position;
}

void LightStorage::pointLightSetRange(PointLight pointLight, float range) {
	CHECK_IF_VALID(_pointLights, pointLight, "PointLight");
	_pointLights[pointLight].range = range;
}

void LightStorage::pointLightSetColor(PointLight pointLight, const glm::vec3 &color) {
	CHECK_IF_VALID(_pointLights, pointLight, "PointLight");
	_pointLights[pointLight].color = color;
}

void LightStorage::pointLightSetIntensity(PointLight pointLight, float intensity) {
	CHECK_IF_VALID(_pointLights, pointLight, "PointLight");
	_pointLights[pointLight].intensity = intensity;
}

void LightStorage::pointLightFree(PointLight pointLight) {
	_pointLights.remove(pointLight);
}

uint32_t LightStorage::getLightCount() const {
	return _pointLights.size();
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

	vk::DescriptorSetLayoutBinding binding = {};
	binding.setBinding(0);
	binding.setDescriptorType(vk::DescriptorType::eStorageBuffer);
	binding.setDescriptorCount(1);
	binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.setBindings(binding);

	vk::Result err = device.createDescriptorSetLayout(&createInfo, nullptr, &_lightSetLayout);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Failed to create light set layout!");

	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.setDescriptorPool(descriptorPool);
	allocInfo.setDescriptorSetCount(1);
	allocInfo.setSetLayouts(_lightSetLayout);

	err = device.allocateDescriptorSets(&allocInfo, &_lightSet);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Failed to allocate light set!");

	vk::BufferUsageFlags usage =
			vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;

	vk::DeviceSize size = sizeof(PointLightRD) * MAX_LIGHT_COUNT;
	_pointlightBuffer = AllocatedBuffer::create(allocator, usage, size, &_pointlightAllocInfo);

	vk::DescriptorBufferInfo bufferInfo = _pointlightBuffer.getBufferInfo();

	vk::WriteDescriptorSet writeInfo = {};
	writeInfo.setDstSet(_lightSet);
	writeInfo.setDstBinding(0);
	writeInfo.setDstArrayElement(0);
	writeInfo.setDescriptorType(vk::DescriptorType::eStorageBuffer);
	writeInfo.setDescriptorCount(1);
	writeInfo.setBufferInfo(bufferInfo);

	device.updateDescriptorSets(writeInfo, nullptr);
}

void LightStorage::update() {
	std::vector<PointLightRD> pointLights(MAX_LIGHT_COUNT);

	uint32_t index = 0;
	for (const auto [_, pointLight] : _pointLights.map()) {
		pointLights[index] = pointLight;
		index++;
	}

	memcpy(_pointlightAllocInfo.pMappedData, pointLights.data(),
			sizeof(PointLightRD) * MAX_LIGHT_COUNT);
}
