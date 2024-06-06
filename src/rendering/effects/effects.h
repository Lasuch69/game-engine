#ifndef EFFECTS_H
#define EFFECTS_H

#include <cstdint>

#include <vulkan/vulkan.hpp>

#include <rendering/types/pipeline.h>

class BRDFEffect {
private:
	Pipeline _pipeline;

public:
	void create(vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass);
	void draw(vk::CommandBuffer commandBuffer);
};

class IrradianceFilterEffect {
private:
	Pipeline _pipeline;

	vk::DescriptorSetLayout _sourceSetLayout;
	vk::DescriptorSet _sourceSet;

public:
	void create(vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass);
	void updateSource(vk::Device device, vk::ImageView imageView, vk::Sampler sampler);
	void draw(vk::CommandBuffer commandBuffer);
};

class SpecularFilterEffect {
private:
	Pipeline _pipeline;

	vk::DescriptorSetLayout _sourceSetLayout;
	vk::DescriptorSet _sourceSet;

	typedef struct {
		uint32_t size;
		float roughness;
	} SpecularFilterConstants;

public:
	void create(vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass);
	void updateSource(vk::Device device, vk::ImageView imageView, vk::Sampler sampler);
	void draw(vk::CommandBuffer commandBuffer, uint32_t size, uint32_t roughness);
};

#endif // !EFFECTS_H
