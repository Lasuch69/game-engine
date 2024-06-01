#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <cstdint>
#include <vulkan/vulkan.hpp>

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

class AllocatedImage;

class EnvironmentEffects {
private:
	vk::Device _device;
	vk::PhysicalDeviceMemoryProperties _memProperties;

	vk::PipelineLayout _brdfPipelineLayout;
	vk::Pipeline _brdfPipeline;

	vk::DescriptorSetLayout _brdfSetLayout;
	vk::DescriptorSet _brdfSet;

	vk::PipelineLayout _cubemapPipelineLayout;
	vk::Pipeline _cubemapPipeline;

	vk::DescriptorSetLayout _cubemapSetLayout;
	vk::DescriptorSet _cubemapSet;

	vk::PipelineLayout _irradiancePipelineLayout;
	vk::Pipeline _irradiancePipeline;

	typedef struct {
		uint32_t size;
		float roughness;
	} SpecularFilterConstants;

	vk::PipelineLayout _specularPipelineLayout;
	vk::Pipeline _specularPipeline;

	vk::DescriptorSetLayout _filterSetLayout;
	vk::DescriptorSet _filterSet;

	bool _initialized = false;

	void _createDescriptors(vk::DescriptorPool descriptorPool);
	void _createPipelines();

	void _updateBrdfSet(vk::ImageView dstImageView);
	void _updateCubemapSet(vk::ImageView srcImageView, vk::ImageView dstCubemapView);
	void _updateFilterSet(vk::ImageView srcImageView, vk::Sampler sampler);

	void _drawIrradianceFilter(vk::CommandBuffer commandBuffer, RenderTarget renderTarget);
	void _drawSpecularFilter(vk::CommandBuffer commandBuffer, RenderTarget renderTarget,
			uint32_t size, float roughness);

	void _copyImageToLevel(vk::Image srcImage, vk::Image dstImage, uint32_t level, uint32_t size);

public:
	AllocatedImage generateBRDF();

	AllocatedImage cubemapCreate(vk::ImageView imageView, uint32_t size);
	AllocatedImage filterIrradiance(vk::ImageView imageView);
	AllocatedImage filterSpecular(vk::ImageView imageView, uint32_t size, uint32_t mipLevels);

	void init();
	~EnvironmentEffects();
};

#endif // !CUBEMAP_H
