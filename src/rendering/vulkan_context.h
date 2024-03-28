#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#include <cstdint>
#include <optional>
#include <vector>

#include <vulkan/vulkan.hpp>

const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapchainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

class VulkanContext {
private:
	void _createSwapchain(uint32_t width, uint32_t height);
	void _destroySwapchain();

public:
	bool validationEnabled = false;

	vk::Instance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

	vk::SurfaceKHR surface;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;

	vk::Queue graphicsQueue;
	vk::Queue presentQueue;

	uint32_t graphicsQueueFamily;

	typedef struct {
		vk::ImageView view;
		vk::Framebuffer framebuffer;
	} SwapchainImageResource;

	std::vector<SwapchainImageResource> swapchainImages;
	vk::SwapchainKHR swapchain;
	vk::Extent2D swapchainExtent;
	vk::RenderPass renderPass;

	vk::Image colorImage;
	vk::ImageView colorView;
	vk::DeviceMemory colorMemory;

	vk::Image depthImage;
	vk::ImageView depthView;
	vk::DeviceMemory depthMemory;

	vk::CommandPool commandPool;

	bool initialized = false;

	void recreateSwapchain(uint32_t width, uint32_t height);
	void initialize(vk::SurfaceKHR surface, uint32_t width, uint32_t height);

	VulkanContext(std::vector<const char *> extensions, bool useValidation);
	~VulkanContext();
};

#endif // !VULKAN_CONTEXT_H
