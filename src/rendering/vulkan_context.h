#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#include <cstdint>
#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "types/attachment.h"

const std::vector<const char *> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char *> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const uint32_t DEPTH_PASS = 0;
const uint32_t MAIN_PASS = 1;
const uint32_t TONEMAP_PASS = 2;

class VulkanContext {
private:
	bool _validation = false;

	vk::Instance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;

	vk::SurfaceKHR _surface;
	vk::PhysicalDevice _physicalDevice;
	vk::Device _device;

	vk::Queue _graphicsQueue;
	vk::Queue _presentQueue;

	uint32_t _graphicsQueueFamily;

	vk::SampleCountFlagBits _msaaSamples = vk::SampleCountFlagBits::e1;

	typedef struct {
		vk::ImageView view;
		vk::Framebuffer framebuffer;
	} SwapchainImageResource;

	std::vector<SwapchainImageResource> _swapchainImages;
	vk::SwapchainKHR _swapchain;
	vk::Extent2D _swapchainExtent;
	vk::RenderPass _renderPass;

	// Color resolve for MSAA
	std::optional<Attachment> _resolve;

	Attachment _color;
	Attachment _depth;

	vk::CommandPool _commandPool;

	bool _initialized = false;

	void _createSwapchain(uint32_t width, uint32_t height);

	void _renderPassCreate(vk::Format format, std::vector<vk::Image> swapchainImages);
	void _renderPassMSAACreate(vk::Format format, std::vector<vk::Image> swapchainImages);

	void _destroySwapchain();

public:
	void initialize(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void recreateSwapchain(uint32_t width, uint32_t height);

	vk::Instance getInstance() const;

	vk::SurfaceKHR getSurface() const;
	vk::PhysicalDevice getPhysicalDevice() const;
	vk::Device getDevice() const;

	vk::Queue getGraphicsQueue() const;
	vk::Queue getPresentQueue() const;

	uint32_t getGraphicsQueueFamily() const;

	void setMSAASamples(vk::SampleCountFlagBits samples);
	vk::SampleCountFlagBits getMSAASamples() const;

	vk::SwapchainKHR getSwapchain() const;
	vk::Extent2D getSwapchainExtent() const;

	vk::RenderPass getRenderPass() const;
	vk::Framebuffer getFramebuffer(uint32_t imageIndex) const;

	Attachment getColorAttachment() const;

	vk::CommandPool getCommandPool() const;

	VulkanContext(std::vector<const char *> extensions, bool validation = false);
	~VulkanContext();
};

#endif // !VULKAN_CONTEXT_H
