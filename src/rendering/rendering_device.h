#ifndef RENDERING_DEVICE_H
#define RENDERING_DEVICE_H

#include <cstdint>
#include <memory>
#include <optional>

#include <glm/glm.hpp>

#include "storage/light_storage.h"
#include "types/allocated.h"
#include "types/resource.h"

#include "effects/environment_effects.h"

#include "vulkan_context.h"

const int FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject {
	glm::vec3 viewPosition;
	uint32_t directionalLightCount;
	uint32_t pointLightCount;
};

struct MeshPushConstants {
	glm::mat4 projView;
	glm::mat4 model;
};

struct TonemapParameterConstants {
	float exposure;
	float white;
};

struct SkyConstants {
	glm::mat4 invProj;
	glm::mat4 invView;
};

class Image;

class RenderingDevice {
public:
	static RenderingDevice &getSingleton() {
		static RenderingDevice instance;
		return instance;
	}

private:
	RenderingDevice() {}

private:
	VulkanContext *_pContext;
	LightStorage _lightStorage;

	uint32_t _frame = 0;

	uint32_t _width, _height;
	bool _resized;

	VmaAllocator _allocator;
	vk::CommandBuffer _commandBuffers[FRAMES_IN_FLIGHT];

	vk::Semaphore _presentSemaphores[FRAMES_IN_FLIGHT];
	vk::Semaphore _renderSemaphores[FRAMES_IN_FLIGHT];
	vk::Fence _fences[FRAMES_IN_FLIGHT];

	vk::DescriptorPool _descriptorPool;

	vk::DescriptorSetLayout _uniformLayout;
	vk::DescriptorSetLayout _inputAttachmentLayout;
	vk::DescriptorSetLayout _textureLayout;
	vk::DescriptorSetLayout _skySetLayout;
	vk::DescriptorSetLayout _iblSetLayout;

	vk::DescriptorSet _uniformSets[FRAMES_IN_FLIGHT];
	vk::DescriptorSet _inputAttachmentSet;
	vk::DescriptorSet _skySet;
	vk::DescriptorSet _iblSet;

	AllocatedBuffer _uniformBuffers[FRAMES_IN_FLIGHT];
	VmaAllocationInfo _uniformAllocInfos[FRAMES_IN_FLIGHT];

	vk::PipelineLayout _depthLayout;
	vk::Pipeline _depthPipeline;

	vk::PipelineLayout _skyLayout;
	vk::Pipeline _skyPipeline;

	vk::PipelineLayout _materialLayout;
	vk::Pipeline _materialPipeline;

	vk::PipelineLayout _tonemapLayout;
	vk::Pipeline _tonemapPipeline;

	std::optional<uint32_t> _imageIndex;

	EnvironmentEffects _environmentEffects;

	float _exposure = 1.25f;
	float _white = 8.0f;

	AllocatedImage _brdfLut;
	vk::ImageView _brdfView;
	vk::Sampler _brdfSampler;

	typedef struct {
		AllocatedImage cubemap;
		vk::ImageView cubemapView;
		vk::Sampler cubemapSampler;

		AllocatedImage irradiance;
		vk::ImageView irradianceView;
		vk::Sampler irradianceSampler;

		AllocatedImage specular;
		vk::ImageView specularView;
		vk::Sampler specularSampler;
	} EnvironmentData;

	EnvironmentData _environmentData;

public:
	RenderingDevice(RenderingDevice const &) = delete;
	void operator=(RenderingDevice const &) = delete;

	vk::CommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

	AllocatedBuffer bufferCreate(
			vk::BufferUsageFlags usage, vk::DeviceSize size, VmaAllocationInfo *pAllocInfo = NULL);
	void bufferCopy(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	void bufferCopyToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height,
			vk::ImageLayout layout = vk::ImageLayout::eTransferDstOptimal);
	void bufferSend(vk::Buffer dstBuffer, uint8_t *pData, size_t size);
	void bufferDestroy(AllocatedBuffer buffer);

	AllocatedImage imageCreate(uint32_t width, uint32_t height, vk::Format format,
			uint32_t mipLevels, vk::ImageUsageFlags usage);
	AllocatedImage imageCubeCreate(
			uint32_t size, vk::Format format, uint32_t mipLevels, vk::ImageUsageFlags usage);
	void imageGenerateMipmaps(vk::Image image, int32_t width, int32_t height, vk::Format format,
			uint32_t mipLevels, uint32_t arrayLayers = 1);
	void imageLayoutTransition(vk::Image image, vk::Format format, uint32_t mipLevels,
			uint32_t arrayLayers, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void imageSend(vk::Image image, uint32_t width, uint32_t height, uint8_t *pData, size_t size,
			vk::ImageLayout layout);
	void imageDestroy(AllocatedImage image);

	vk::ImageView imageViewCreate(vk::Image image, vk::Format format, uint32_t mipLevels,
			uint32_t arrayLayers = 1, vk::ImageViewType viewType = vk::ImageViewType::e2D);
	void imageViewDestroy(vk::ImageView imageView);

	vk::Sampler samplerCreate(vk::Filter filter, uint32_t mipLevels, float mipLodBias = 0.0f);
	void samplerDestroy(vk::Sampler sampler);

	TextureRD textureCreate(const std::shared_ptr<Image> image);
	void textureDestroy(TextureRD texture);

	void environmentSkyUpdate(const std::shared_ptr<Image> image);

	void updateUniformBuffer(const glm::vec3 &viewPosition);

	LightStorage &getLightStorage();

	vk::Instance getInstance() const;
	vk::PhysicalDevice getPhysicalDevice() const;
	vk::Device getDevice() const;

	vk::Extent2D getSwapchainExtent() const;

	vk::PipelineLayout getDepthPipelineLayout() const;
	vk::Pipeline getDepthPipeline() const;

	vk::PipelineLayout getSkyPipelineLayout() const;
	vk::Pipeline getSkyPipeline() const;

	vk::DescriptorSet getSkySet() const;

	vk::PipelineLayout getMaterialPipelineLayout() const;
	vk::Pipeline getMaterialPipeline() const;

	std::array<vk::DescriptorSet, 3> getMaterialSets() const;

	vk::DescriptorPool getDescriptorPool() const;
	vk::DescriptorSetLayout getTextureLayout() const;

	void setExposure(float exposure);
	void setWhite(float white);

	vk::CommandBuffer drawBegin();
	void drawEnd(vk::CommandBuffer commandBuffer);

	void windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void windowResize(uint32_t width, uint32_t height);

	void init(std::vector<const char *> extensions, bool useValidation);
};

typedef RenderingDevice RD;

#endif // !RENDERING_DEVICE_H
