#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <cstdint>
#include <vulkan/vulkan.hpp>

class RenderTarget;

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

	struct SpecularFilterConstants {
		uint32_t size;
		float roughness;
	};

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
	void generateBrdf(vk::ImageView dstImageView, uint32_t size);
	void imageCopyToCube(vk::ImageView srcImageView, vk::ImageView dstCubemapView, uint32_t size);

	void filterIrradiance(
			vk::ImageView srcCubemapView, vk::Image dstCubemap, vk::Format format, uint32_t size);
	void filterSpecular(vk::ImageView srcCubemapView, uint32_t srcSize, uint32_t srcMipLevels,
			vk::Format format, vk::Image dstCubemap, uint32_t dstSize, uint32_t dstMipLevels);

	void init();
	~EnvironmentEffects();
};

#endif // !CUBEMAP_H
