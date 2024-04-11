#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

const std::vector<const char *> VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char *> DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

struct QueueFamilyIndices {
	uint32_t graphicsFamily = UINT32_MAX;
	uint32_t presentFamily = UINT32_MAX;

	bool isComplete() {
		return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
	}
};

struct SwapchainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> surfaceFormats;
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
