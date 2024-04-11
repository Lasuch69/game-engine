#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

#include <version.h>

#include "vulkan_context.h"

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

	std::cout << pCallbackData->pMessage << std::endl;
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

	vk::InstanceCreateInfo createInfo = vk::InstanceCreateInfo()
												.setPApplicationInfo(&appInfo)
												.setEnabledExtensionCount(extensions.size())
												.setPpEnabledExtensionNames(extensions.data());

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

	if (useValidation) {
		debugCreateInfo = {};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;

		createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();

		createInfo.setEnabledLayerCount(VALIDATION_LAYERS.size());
		createInfo.setPpEnabledLayerNames(VALIDATION_LAYERS.data());
		createInfo.setPNext((VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo);
	}

	vk::Instance instance = vk::createInstance(createInfo);

	if (useValidation) {
		VkResult err =
				CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, pDebugMessenger);

		if (err != VK_SUCCESS)
			std::cout << "Failed to set up debug messenger!" << std::endl;
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
	std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

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

	return { capabilities, formats, presentModes };
}

bool isDeviceSuitable(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapchainSupportDetails swapChainSupport = querySwapchainSupport(physicalDevice, surface);
		swapChainAdequate =
				!swapChainSupport.surfaceFormats.empty() && !swapChainSupport.presentModes.empty();
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

	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily,
		indices.presentFamily,
	};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo()
															.setQueueFamilyIndex(queueFamily)
															.setQueueCount(1)
															.setQueuePriorities(queuePriority);

		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::PhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo()
											  .setQueueCreateInfos(queueCreateInfos)
											  .setPEnabledFeatures(&deviceFeatures)
											  .setEnabledExtensionCount(DEVICE_EXTENSIONS.size())
											  .setPpEnabledExtensionNames(DEVICE_EXTENSIONS.data());

	if (useValidation) {
		createInfo.setEnabledLayerCount(VALIDATION_LAYERS.size());
		createInfo.setPpEnabledLayerNames(VALIDATION_LAYERS.data());
	}

	return physicalDevice.createDevice(createInfo);
}

bool checkValidationLayerSupport() {
	std::vector<vk::LayerProperties> layers = vk::enumerateInstanceLayerProperties();
	for (const char *layerName : VALIDATION_LAYERS) {
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

	throw std::runtime_error("Failed to find suitable memory type!");
}

vk::Image createImage(vk::Device device, uint32_t width, uint32_t height, vk::Format format,
		vk::ImageUsageFlags usage, vk::PhysicalDeviceMemoryProperties memProperties,
		vk::DeviceMemory *pMemory) {
	vk::ImageCreateInfo createInfo = vk::ImageCreateInfo()
											 .setImageType(vk::ImageType::e2D)
											 .setExtent(vk::Extent3D(width, height, 1))
											 .setMipLevels(1)
											 .setArrayLayers(1)
											 .setFormat(format)
											 .setTiling(vk::ImageTiling::eOptimal)
											 .setInitialLayout(vk::ImageLayout::eUndefined)
											 .setUsage(usage)
											 .setSamples(vk::SampleCountFlagBits::e1)
											 .setSharingMode(vk::SharingMode::eExclusive);

	vk::Image image = device.createImage(createInfo);

	vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image);

	uint32_t memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, memProperties,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo()
											   .setAllocationSize(memRequirements.size)
											   .setMemoryTypeIndex(memoryTypeIndex);

	vk::Result err = device.allocateMemory(&allocInfo, nullptr, pMemory);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Failed to allocate image memory!");

	device.bindImageMemory(image, *pMemory, 0);
	return image;
}

vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format,
		vk::ImageAspectFlagBits aspectFlags) {
	vk::ImageSubresourceRange subresourceRange = vk::ImageSubresourceRange()
														 .setAspectMask(aspectFlags)
														 .setBaseMipLevel(0)
														 .setLevelCount(1)
														 .setBaseArrayLayer(0)
														 .setLayerCount(1);

	vk::ImageViewCreateInfo createInfo = vk::ImageViewCreateInfo()
												 .setImage(image)
												 .setViewType(vk::ImageViewType::e2D)
												 .setFormat(format)
												 .setSubresourceRange(subresourceRange);

	return device.createImageView(createInfo);
}

vk::SurfaceFormatKHR getSurfaceFormat(std::vector<vk::SurfaceFormatKHR> surfaceFormats) {
	assert(!surfaceFormats.empty());

	for (const vk::SurfaceFormatKHR &surfaceFormat : surfaceFormats) {
		if (surfaceFormat.format != vk::Format::eB8G8R8A8Srgb ||
				surfaceFormat.colorSpace != vk::ColorSpaceKHR::eSrgbNonlinear)
			continue;

		return surfaceFormat;
	}

	return surfaceFormats[0];
}

vk::PresentModeKHR choosePresentMode(std::vector<vk::PresentModeKHR> presentModes,
		vk::PresentModeKHR desiredMode = vk::PresentModeKHR::eFifo) {
	assert(!presentModes.empty());

	for (const vk::PresentModeKHR &presentMode : presentModes) {
		if (presentMode != desiredMode)
			continue;

		return presentMode;
	}

	return vk::PresentModeKHR::eFifo;
}

void VulkanContext::_createSwapchain(uint32_t width, uint32_t height) {
	SwapchainSupportDetails support = querySwapchainSupport(physicalDevice, surface);

	if (support.capabilities.currentExtent.width == UINT32_MAX) {
		vk::Extent2D min = support.capabilities.minImageExtent;
		vk::Extent2D max = support.capabilities.maxImageExtent;

		uint32_t _width = std::clamp(width, min.width, max.width);
		uint32_t _height = std::clamp(height, min.height, max.height);

		swapchainExtent = vk::Extent2D(_width, _height);
	} else {
		swapchainExtent = support.capabilities.currentExtent;
	}

	uint32_t minImageCount = support.capabilities.minImageCount + 1;
	if (support.capabilities.maxImageCount > 0 &&
			minImageCount > support.capabilities.maxImageCount) {
		minImageCount = support.capabilities.maxImageCount;
	}

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	vk::SharingMode sharingMode = vk::SharingMode::eExclusive;

	std::array<uint32_t, 2> queueFamilyIndices = {};

	if (indices.graphicsFamily != indices.presentFamily) {
		sharingMode = vk::SharingMode::eConcurrent;

		queueFamilyIndices[0] = indices.graphicsFamily;
		queueFamilyIndices[1] = indices.presentFamily;
	}

	vk::SurfaceFormatKHR surfaceFormat = getSurfaceFormat(support.surfaceFormats);

	vk::PresentModeKHR presentMode =
			choosePresentMode(support.presentModes, vk::PresentModeKHR::eMailbox);

	vk::SwapchainCreateInfoKHR createInfo =
			vk::SwapchainCreateInfoKHR()
					.setSurface(surface)
					.setMinImageCount(minImageCount)
					.setImageFormat(surfaceFormat.format)
					.setImageColorSpace(surfaceFormat.colorSpace)
					.setImageExtent(swapchainExtent)
					.setImageArrayLayers(1)
					.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
					.setImageSharingMode(sharingMode)
					.setQueueFamilyIndices(queueFamilyIndices)
					.setPreTransform(support.capabilities.currentTransform)
					.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
					.setPresentMode(presentMode)
					.setClipped(true);

	swapchain = device.createSwapchainKHR(createInfo);

	std::vector<vk::Image> images = device.getSwapchainImagesKHR(swapchain);
	swapchainImages.resize(images.size());

	// Resources

	vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

	vk::Format colorFormat = vk::Format::eR16G16B16A16Sfloat;
	colorImage = createImage(device, swapchainExtent.width, swapchainExtent.height, colorFormat,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment,
			memProperties, &colorMemory);
	colorView = createImageView(device, colorImage, colorFormat, vk::ImageAspectFlagBits::eColor);

	vk::Format depthFormat = vk::Format::eD32Sfloat;
	depthImage = createImage(device, swapchainExtent.width, swapchainExtent.height, depthFormat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment, memProperties, &depthMemory);
	depthView = createImageView(device, depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

	// Framebuffers

	std::array<vk::AttachmentDescription, 3> attachments = {
		vk::AttachmentDescription() // Present
				.setFormat(surfaceFormat.format)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::ePresentSrcKHR),

		vk::AttachmentDescription() // Color
				.setFormat(colorFormat)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal),

		vk::AttachmentDescription() // Depth
				.setFormat(depthFormat)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal),
	};

	std::array<vk::AttachmentReference, 4> attachmentReferences{
		// Present 0
		vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),

		// Color 1 - 2
		vk::AttachmentReference(1, vk::ImageLayout::eColorAttachmentOptimal),
		vk::AttachmentReference(1, vk::ImageLayout::eShaderReadOnlyOptimal),

		// Depth 3
		vk::AttachmentReference(2, vk::ImageLayout::eDepthStencilAttachmentOptimal),
	};

	std::array<vk::SubpassDescription, 2> subpasses = {
		vk::SubpassDescription() // Geometry - 0
				.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
				.setColorAttachments(attachmentReferences[1])
				.setPDepthStencilAttachment(&attachmentReferences[3]),

		vk::SubpassDescription() // Tonemapping - 1
				.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
				.setColorAttachments(attachmentReferences[0])
				.setInputAttachments(attachmentReferences[2]),
	};

	std::array<vk::SubpassDependency, 2> dependencies = {
		vk::SubpassDependency() // Geometry - 0
				.setSrcSubpass(VK_SUBPASS_EXTERNAL)
				.setDstSubpass(0)
				.setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
				.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				.setSrcAccessMask(vk::AccessFlagBits::eNone)
				.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite),
		vk::SubpassDependency() // Tonemapping - 1
				.setSrcSubpass(0)
				.setDstSubpass(1)
				.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
				.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
				.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead),
	};

	vk::RenderPassCreateInfo renderPassInfo = vk::RenderPassCreateInfo()
													  .setAttachments(attachments)
													  .setSubpasses(subpasses)
													  .setDependencies(dependencies);

	renderPass = device.createRenderPass(renderPassInfo);

	for (size_t i = 0; i < images.size(); i++) {
		vk::ImageView presentView = createImageView(
				device, images[i], surfaceFormat.format, vk::ImageAspectFlagBits::eColor);

		std::array<vk::ImageView, 3> attachmentViews = {
			presentView,
			colorView,
			depthView,
		};

		vk::FramebufferCreateInfo framebufferInfo = vk::FramebufferCreateInfo()
															.setRenderPass(renderPass)
															.setAttachments(attachmentViews)
															.setWidth(swapchainExtent.width)
															.setHeight(swapchainExtent.height)
															.setLayers(1);

		vk::Framebuffer framebuffer;
		vk::Result err = device.createFramebuffer(&framebufferInfo, nullptr, &framebuffer);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create framebuffer!");

		swapchainImages[i] = { presentView, framebuffer };
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
	graphicsQueue = device.getQueue(indices.graphicsFamily, 0);
	presentQueue = device.getQueue(indices.presentFamily, 0);

	graphicsQueueFamily = indices.graphicsFamily;

	_createSwapchain(width, height);

	vk::CommandPoolCreateInfo createInfo =
			vk::CommandPoolCreateInfo()
					.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
					.setQueueFamilyIndex(graphicsQueueFamily);

	commandPool = device.createCommandPool(createInfo);

	initialized = true;
}

VulkanContext::VulkanContext(std::vector<const char *> extensions, bool useValidation) {
	if (useValidation && !checkValidationLayerSupport()) {
		std::cout << "Validation not supported!" << std::endl;
		useValidation = false;
	}

	validationEnabled = useValidation;
	instance = createInstance(extensions, useValidation, &debugMessenger);
}

VulkanContext::~VulkanContext() {}
