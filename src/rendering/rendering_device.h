#ifndef RENDERING_DEVICE_H
#define RENDERING_DEVICE_H

#include <cstdint>
#include <optional>

#include "types/allocated.h"
#include "vulkan_context.h"

const uint32_t FRAMES_IN_FLIGHT = 2;

class RenderingDevice {
public:
	static RenderingDevice &getSingleton() {
		static RenderingDevice instance;
		return instance;
	}

private:
	RenderingDevice() {}

	VulkanContext *_pContext;
	VmaAllocator _allocator;

	vk::CommandBuffer _commandBuffers[FRAMES_IN_FLIGHT];

	vk::Semaphore _presentSemaphores[FRAMES_IN_FLIGHT];
	vk::Semaphore _renderSemaphores[FRAMES_IN_FLIGHT];
	vk::Fence _fences[FRAMES_IN_FLIGHT];

	vk::DescriptorPool _descriptorPool;

	std::optional<uint32_t> _imageIndex;

	uint32_t _frame = 0;
	uint32_t _width, _height;
	bool _resized;

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
	void imageLayoutTransition(vk::Image image, vk::Format format, uint32_t mipLevels,
			uint32_t arrayLayers, vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
	void imageSend(vk::Image image, uint32_t width, uint32_t height, uint8_t *pData, size_t size,
			vk::ImageLayout layout);
	void imageGenerateMipmaps(vk::Image image, int32_t width, int32_t height, vk::Format format,
			uint32_t mipLevels, uint32_t arrayLayers);
	void imageDestroy(AllocatedImage image);

	vk::ImageView imageViewCreate(vk::Image image, vk::Format format, uint32_t mipLevels,
			uint32_t arrayLayers = 1, vk::ImageViewType viewType = vk::ImageViewType::e2D);
	void imageViewDestroy(vk::ImageView imageView);

	vk::Sampler samplerCreate(vk::Filter filter, uint32_t mipLevels, float mipLodBias = 0.0f);
	void samplerDestroy(vk::Sampler sampler);

	vk::CommandBuffer drawBegin();
	void drawEnd(vk::CommandBuffer commandBuffer);

	void initImGui();

	void windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void windowResize(uint32_t width, uint32_t height);

	vk::Instance getInstance() const;
	vk::PhysicalDevice getPhysicalDevice() const;
	vk::PhysicalDeviceMemoryProperties getMemoryProperties() const;

	vk::Device getDevice() const;
	vk::DescriptorPool getDescriptorPool() const;

	vk::Extent2D getSwapchainExtent() const;
	vk::RenderPass getRenderPass() const;
	vk::Framebuffer getFramebuffer() const;

	uint32_t getFrameIndex() const;

	void init(std::vector<const char *> extensions, bool useValidation);
};

typedef RenderingDevice RD;

#endif // !RENDERING_DEVICE_H
