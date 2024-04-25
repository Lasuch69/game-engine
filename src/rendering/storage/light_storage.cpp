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

DirectionalLight LightStorage::directionalLightCreate() {
	return _directionalLights.insert({});
}

void LightStorage::directionalLightSetDirection(
		DirectionalLight directionalLight, const glm::vec3 &direction) {
	CHECK_IF_VALID(_directionalLights, directionalLight, "DirectionalLight");
	_directionalLights[directionalLight].direction = direction;
}

void LightStorage::directionalLightSetColor(
		DirectionalLight directionalLight, const glm::vec3 &color) {
	CHECK_IF_VALID(_directionalLights, directionalLight, "DirectionalLight");
	_directionalLights[directionalLight].color = color;
}

void LightStorage::directionalLightSetIntensity(
		DirectionalLight directionalLight, float intensity) {
	CHECK_IF_VALID(_directionalLights, directionalLight, "DirectionalLight");
	_directionalLights[directionalLight].intensity = intensity;
}

void LightStorage::directionalLightFree(DirectionalLight directionalLight) {
	_directionalLights.remove(directionalLight);
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

uint32_t LightStorage::getDirectionalLightCount() const {
	return _directionalLights.size();
}

uint32_t LightStorage::getPointLightCount() const {
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

	_directionalLightBuffer = AllocatedBuffer::create(allocator, usage,
			sizeof(DirectionalLightRD) * MAX_DIRECTIONAL_LIGHT_COUNT, &_directionalLightAllocInfo);

	_pointLightBuffer = AllocatedBuffer::create(
			allocator, usage, sizeof(PointLightRD) * MAX_POINT_LIGHT_COUNT, &_pointLightAllocInfo);

	vk::DescriptorBufferInfo directionalLightBufferInfo = _directionalLightBuffer.getBufferInfo();
	vk::DescriptorBufferInfo pointLightBufferInfo = _pointLightBuffer.getBufferInfo();

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
	{
		std::vector<DirectionalLightRD> directionalLights(MAX_DIRECTIONAL_LIGHT_COUNT);

		uint32_t index = 0;
		for (const auto [_, directionalLight] : _directionalLights.map()) {
			if (index >= MAX_DIRECTIONAL_LIGHT_COUNT)
				break;

			directionalLights[index] = directionalLight;
			index++;
		}

		memcpy(_directionalLightAllocInfo.pMappedData, directionalLights.data(),
				sizeof(DirectionalLightRD) * MAX_DIRECTIONAL_LIGHT_COUNT);
	}

	{
		std::vector<PointLightRD> pointLights(MAX_POINT_LIGHT_COUNT);

		uint32_t index = 0;
		for (const auto [_, pointLight] : _pointLights.map()) {
			if (index >= MAX_POINT_LIGHT_COUNT)
				break;

			pointLights[index] = pointLight;
			index++;
		}

		memcpy(_pointLightAllocInfo.pMappedData, pointLights.data(),
				sizeof(PointLightRD) * MAX_POINT_LIGHT_COUNT);
	}
}
