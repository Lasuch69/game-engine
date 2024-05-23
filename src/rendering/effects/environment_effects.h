#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <cstdint>
#include <vulkan/vulkan.hpp>

#include "../types/allocated.h"
#include "../types/attachment.h"

class RenderTarget {
private:
	vk::Framebuffer _framebuffer;
	vk::RenderPass _renderPass;

	Attachment _color;
	uint32_t _size;

public:
	static RenderTarget create(
			vk::Device device, uint32_t size, vk::PhysicalDeviceMemoryProperties memProperties);

	void destroy(vk::Device device);

	vk::Framebuffer getFramebuffer() const;
	vk::RenderPass getRenderPass() const;
	Attachment getColorAttachment() const;
	uint32_t getSize() const;

	RenderTarget(vk::Framebuffer framebuffer, vk::RenderPass renderPass, Attachment color,
			uint32_t size);
};

class EnvironmentEffects {
private:
	vk::PipelineLayout _cubemapPipelineLayout;
	vk::Pipeline _cubemapPipeline;

	vk::DescriptorSetLayout _cubemapSetLayout;
	vk::DescriptorSet _cubemapSet;

	vk::PipelineLayout _brdfPipelineLayout;
	vk::Pipeline _brdfPipeline;

	vk::DescriptorSetLayout _brdfSetLayout;
	vk::DescriptorSet _brdfSet;

	vk::PipelineLayout _irradianceFilterPipelineLayout;
	vk::Pipeline _irradianceFilterPipeline;

	struct SpecularFilterConstants {
		uint32_t srcCubeSize;
		float roughness;
	};

	vk::PipelineLayout _specularFilterPipelineLayout;
	vk::Pipeline _specularFilterPipeline;

	vk::DescriptorSetLayout _filterSetLayout;
	vk::DescriptorSet _filterSet;

	bool _initialized = false;

	void _createDescriptors(vk::Device device, vk::DescriptorPool descriptorPool);
	void _createPipelines(vk::Device device, vk::PhysicalDeviceMemoryProperties memProperties);

	void _updateCubemapSet(vk::Device device, vk::ImageView imageView, vk::ImageView cubeView);
	void _updateBrdfSet(vk::Device device, vk::ImageView imageView);
	void _updateFilterSet(vk::Device device, vk::ImageView view, vk::Sampler sampler);

	void _filterIrradiance(vk::CommandBuffer commandBuffer, RenderTarget renderTarget);
	void _filterSpecularStep(vk::CommandBuffer commandBuffer, RenderTarget renderTarget,
			uint32_t srcCubeSize, float roughness);

public:
	void imageCopyToCube(vk::ImageView image, vk::ImageView cube, uint32_t cubeSize);
	void generateBrdf(vk::ImageView image, uint32_t size);

	AllocatedImage filterIrradiance(vk::ImageView cubeView);
	AllocatedImage filterSpecular(vk::ImageView cubeView, uint32_t srcCubeSize, uint32_t mipLevels);

	void init();
	~EnvironmentEffects();
};

#endif // !CUBEMAP_H
