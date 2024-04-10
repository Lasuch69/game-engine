#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <string>

#include "vulkan_context.h"

#include <version.h>

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
		const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks *pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
			instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		return VK_FALSE;
	}

	printf("%s\n", pCallbackData->pMessage);
	return VK_FALSE;
}

vk::Instance createInstance(std::vector<const char *> extensions, bool useValidation,
		VkDebugUtilsMessengerEXT *pDebugMessenger) {
	uint32_t version = VK_MAKE_VERSION(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);

	vk::ApplicationInfo appInfo{};
	appInfo.setPApplicationName("Hayaku");
	appInfo.setApplicationVersion(version);
	appInfo.setPEngineName("Hayaku Engine");
	appInfo.setEngineVersion(version);
	appInfo.setApiVersion(VK_API_VERSION_1_1);

	if (useValidation) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	vk::InstanceCreateInfo createInfo{};
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (useValidation) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		debugCreateInfo = {};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	vk::Instance instance;
	if (vk::createInstance(&createInfo, nullptr, &instance) != vk::Result::eSuccess) {
		printf("Failed to create instance!\n");
	}

	if (useValidation) {
		VkResult err =
				CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, pDebugMessenger);

		if (err != VK_SUCCESS)
			printf("Failed to set up debug messenger!\n");
	}

	return instance;
}

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {
	QueueFamilyIndices indices;

	std::vector<vk::QueueFamilyProperties> queueFamilies =
			physicalDevice.getQueueFamilyProperties();

	int i = 0;
	for (const vk::QueueFamilyProperties &queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

bool checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice) {
	std::vector<vk::ExtensionProperties> extensions =
			physicalDevice.enumerateDeviceExtensionProperties();
	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto &extension : extensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapchainSupportDetails querySwapchainSupport(
		vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {
	vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
	std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
	std::vector<vk::PresentModeKHR> presentModes =
			physicalDevice.getSurfacePresentModesKHR(surface);

	return SwapchainSupportDetails{ capabilities, formats, presentModes };
}

bool isDeviceSuitable(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapchainSupportDetails swapChainSupport = querySwapchainSupport(physicalDevice, surface);
		swapChainAdequate =
				!swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	vk::PhysicalDeviceFeatures supportedFeatures = physicalDevice.getFeatures();

	return indices.isComplete() && extensionsSupported && swapChainAdequate &&
		   supportedFeatures.samplerAnisotropy;
}

vk::PhysicalDevice pickPhysicalDevice(vk::Instance instance, vk::SurfaceKHR surface) {
	vk::PhysicalDevice chosenDevice = VK_NULL_HANDLE;
	std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

	if (devices.empty()) {
		return chosenDevice;
	}

	for (const vk::PhysicalDevice &device : devices) {
		if (isDeviceSuitable(device, surface)) {
			chosenDevice = device;
			break;
		}
	}

	return chosenDevice;
}

vk::Device createDevice(
		vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, bool useValidation) {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(),
		indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ {}, queueFamily, 1, &queuePriority });
	}

	vk::PhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	vk::DeviceCreateInfo createInfo{ {}, static_cast<uint32_t>(queueCreateInfos.size()),
		queueCreateInfos.data(), 0, {}, static_cast<uint32_t>(deviceExtensions.size()),
		deviceExtensions.data(), &deviceFeatures };

	if (useValidation) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}

	return physicalDevice.createDevice(createInfo);
}

bool checkValidationLayerSupport() {
	std::vector<vk::LayerProperties> layers = vk::enumerateInstanceLayerProperties();
	for (const char *layerName : validationLayers) {
		bool layerFound = false;

		for (const auto &layerProperties : layers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

uint32_t findMemoryType(uint32_t typeFilter, vk::PhysicalDeviceMemoryProperties memProperties,
		vk::MemoryPropertyFlags properties) {
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&
				(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	printf("Failed to find suitable memory type!\n");
	return 0;
}

vk::Image createImage(vk::Device device, uint32_t width, uint32_t height, vk::Format format,
		vk::ImageUsageFlags usage, vk::PhysicalDeviceMemoryProperties memProperties,
		vk::DeviceMemory *pMemory) {
	vk::ImageCreateInfo createInfo{ {}, vk::ImageType::e2D, format,
		vk::Extent3D{ width, height, 1 }, 1, 1, vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal, usage, vk::SharingMode::eExclusive };

	vk::Image image;
	vk::Result err = device.createImage(&createInfo, nullptr, &image);

	if (err != vk::Result::eSuccess) {
		printf("Failed to create image!\n");
	}

	vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image);

	vk::MemoryAllocateInfo allocInfo{ memRequirements.size,
		findMemoryType(memRequirements.memoryTypeBits, memProperties,
				vk::MemoryPropertyFlagBits::eDeviceLocal) };

	err = device.allocateMemory(&allocInfo, nullptr, pMemory);

	if (err != vk::Result::eSuccess) {
		printf("Failed to allocate image memory!\n");
	}

	device.bindImageMemory(image, *pMemory, 0);
	return image;
}

vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format,
		vk::ImageAspectFlagBits aspectFlags) {
	vk::ImageViewCreateInfo createInfo{ {}, image, vk::ImageViewType::e2D, format, {},
		vk::ImageSubresourceRange{ aspectFlags, 0, 1, 0, 1 } };

	vk::ImageView imageView;
	vk::Result err = device.createImageView(&createInfo, nullptr, &imageView);

	if (err != vk::Result::eSuccess) {
		printf("Failed to create image view!\n");
	}

	return imageView;
}

void VulkanContext::_createSwapchain(uint32_t width, uint32_t height) {
	SwapchainSupportDetails support = querySwapchainSupport(physicalDevice, surface);

	vk::SurfaceFormatKHR surfaceFormat = support.formats[0];
	for (const vk::SurfaceFormatKHR &availableFormat : support.formats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
				availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			surfaceFormat = availableFormat;
		}
	}

	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	for (const vk::PresentModeKHR &availableMode : support.presentModes) {
		if (availableMode == vk::PresentModeKHR::eMailbox) {
			presentMode = availableMode;
		}
	}

	vk::Extent2D extent = support.capabilities.currentExtent;
	if (extent.width == std::numeric_limits<uint32_t>::max()) {
		vk::Extent2D min = support.capabilities.minImageExtent;
		vk::Extent2D max = support.capabilities.maxImageExtent;

		extent = vk::Extent2D{ std::clamp(width, min.width, max.width),
			std::clamp(height, min.height, max.height) };
	}

	swapchainExtent = extent;

	uint32_t imageCount = support.capabilities.minImageCount + 1;
	if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
		imageCount = support.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo{ {}, surface, imageCount, surfaceFormat.format,
		surfaceFormat.colorSpace, extent, 1, vk::ImageUsageFlagBits::eColorAttachment };

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(),
		indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}

	createInfo.preTransform = support.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	swapchain = device.createSwapchainKHR(createInfo);
	std::vector<vk::Image> images = device.getSwapchainImagesKHR(swapchain);

	// Resources

	vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

	vk::Format colorFormat = vk::Format::eR16G16B16A16Sfloat;
	colorImage = createImage(device, extent.width, extent.height, colorFormat,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment,
			memProperties, &colorMemory);
	colorView = createImageView(device, colorImage, colorFormat, vk::ImageAspectFlagBits::eColor);

	vk::Format depthFormat = vk::Format::eD32Sfloat;
	depthImage = createImage(device, extent.width, extent.height, depthFormat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment, memProperties, &depthMemory);
	depthView = createImageView(device, depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

	// Framebuffers

	vk::AttachmentDescription finalColorAttachment{ {}, surfaceFormat.format,
		vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR };

	vk::AttachmentDescription colorAttachment{ {}, colorFormat, vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal };

	vk::AttachmentDescription depthAttachment{ {}, depthFormat, vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eDontCare,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal };

	vk::AttachmentReference finalColorAttachmentRef{ 0, vk::ImageLayout::eColorAttachmentOptimal };
	vk::AttachmentReference colorAttachmentRef{ 1, vk::ImageLayout::eColorAttachmentOptimal };
	vk::AttachmentReference depthAttachmentRef{ 2,
		vk::ImageLayout::eDepthStencilAttachmentOptimal };

	vk::AttachmentReference inputAttachmentRef{ 1, vk::ImageLayout::eShaderReadOnlyOptimal };

	vk::SubpassDescription finalSubpass{ {}, vk::PipelineBindPoint::eGraphics, 1,
		&inputAttachmentRef, 1, &finalColorAttachmentRef };

	vk::SubpassDescription mainSubpass{ {}, vk::PipelineBindPoint::eGraphics, {}, {}, 1,
		&colorAttachmentRef, {}, &depthAttachmentRef };

	vk::SubpassDependency dependencies[2] = {
		{ VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eBottomOfPipe,
				vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eNone,
				vk::AccessFlagBits::eColorAttachmentWrite },
		{ 0, 1, vk::PipelineStageFlagBits::eColorAttachmentOutput,
				vk::PipelineStageFlagBits::eFragmentShader,
				vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eShaderRead }
	};

	vk::AttachmentDescription attachments[] = {
		finalColorAttachment,
		colorAttachment,
		depthAttachment,
	};

	vk::SubpassDescription subpasses[] = { mainSubpass, finalSubpass };

	vk::RenderPassCreateInfo renderPassInfo{ {}, 3, attachments, 2, subpasses, 2, dependencies };
	renderPass = device.createRenderPass(renderPassInfo);

	swapchainImages.resize(images.size());
	for (size_t i = 0; i < images.size(); i++) {
		vk::ImageView view = createImageView(
				device, images[i], surfaceFormat.format, vk::ImageAspectFlagBits::eColor);

		vk::ImageView attachmentViews[] = {
			view,
			colorView,
			depthView,
		};

		vk::FramebufferCreateInfo createInfo{ {}, renderPass, 3, attachmentViews, extent.width,
			extent.height, 1 };

		vk::Framebuffer framebuffer;
		vk::Result err = device.createFramebuffer(&createInfo, nullptr, &framebuffer);

		if (err != vk::Result::eSuccess) {
			printf("Failed to create framebuffer!\n");
		}

		swapchainImages[i] = { view, framebuffer };
	}
}

void VulkanContext::_destroySwapchain() {
	device.destroyImageView(colorView, nullptr);
	device.destroyImage(colorImage, nullptr);
	device.freeMemory(colorMemory, nullptr);

	device.destroyImageView(depthView, nullptr);
	device.destroyImage(depthImage, nullptr);
	device.freeMemory(depthMemory, nullptr);

	for (uint32_t i = 0; i < swapchainImages.size(); i++) {
		device.destroyFramebuffer(swapchainImages[i].framebuffer, nullptr);
		device.destroyImageView(swapchainImages[i].view, nullptr);
	}
	swapchainImages.clear();

	device.destroySwapchainKHR(swapchain, nullptr);
	device.destroyRenderPass(renderPass, nullptr);
}

void VulkanContext::recreateSwapchain(uint32_t width, uint32_t height) {
	device.waitIdle();

	_destroySwapchain();
	_createSwapchain(width, height);
}

void VulkanContext::initialize(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
	this->surface = surface;

	physicalDevice = pickPhysicalDevice(instance, surface);

	vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

	std::cout << properties.deviceName << std::endl;

	device = createDevice(physicalDevice, surface, validationEnabled);

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
	presentQueue = device.getQueue(indices.presentFamily.value(), 0);

	graphicsQueueFamily = indices.graphicsFamily.value();

	_createSwapchain(width, height);

	vk::CommandPoolCreateInfo createInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		graphicsQueueFamily };
	commandPool = device.createCommandPool(createInfo);

	initialized = true;
}

VulkanContext::VulkanContext(std::vector<const char *> extensions, bool useValidation) {
	if (useValidation && !checkValidationLayerSupport()) {
		printf("Validation not supported!\n");
		useValidation = false;
	}

	validationEnabled = useValidation;
	instance = createInstance(extensions, useValidation, &debugMessenger);
}

VulkanContext::~VulkanContext() {}
