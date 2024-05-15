#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <string>

#include <version.h>

#include <SDL2/SDL_log.h>

#include "vulkan_context.h"

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

	SDL_LogCritical(SDL_LOG_CATEGORY_RENDER, "%s", pCallbackData->pMessage);
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

	vk::InstanceCreateInfo createInfo = {};
	createInfo.setPApplicationInfo(&appInfo);
	createInfo.setEnabledExtensionCount(extensions.size());
	createInfo.setPpEnabledExtensionNames(extensions.data());

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
			SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Validation setup failed!");
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
		vk::DeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.setQueueFamilyIndex(queueFamily);
		queueCreateInfo.setQueueCount(1);
		queueCreateInfo.setQueuePriorities(queuePriority);

		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::PhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	vk::DeviceCreateInfo createInfo = {};
	createInfo.setQueueCreateInfos(queueCreateInfos);
	createInfo.setPEnabledFeatures(&deviceFeatures);
	createInfo.setEnabledExtensionCount(DEVICE_EXTENSIONS.size());
	createInfo.setPpEnabledExtensionNames(DEVICE_EXTENSIONS.data());

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
	SwapchainSupportDetails support = querySwapchainSupport(_physicalDevice, _surface);

	if (support.capabilities.currentExtent.width == UINT32_MAX) {
		vk::Extent2D min = support.capabilities.minImageExtent;
		vk::Extent2D max = support.capabilities.maxImageExtent;

		uint32_t _width = std::clamp(width, min.width, max.width);
		uint32_t _height = std::clamp(height, min.height, max.height);

		_swapchainExtent = vk::Extent2D(_width, _height);
	} else {
		_swapchainExtent = support.capabilities.currentExtent;
	}

	uint32_t minImageCount = support.capabilities.minImageCount + 1;
	if (support.capabilities.maxImageCount > 0 &&
			minImageCount > support.capabilities.maxImageCount) {
		minImageCount = support.capabilities.maxImageCount;
	}

	QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, _surface);
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

	vk::SwapchainCreateInfoKHR createInfo = {};
	createInfo.setSurface(_surface);
	createInfo.setMinImageCount(minImageCount);
	createInfo.setImageFormat(surfaceFormat.format);
	createInfo.setImageColorSpace(surfaceFormat.colorSpace);
	createInfo.setImageExtent(_swapchainExtent);
	createInfo.setImageArrayLayers(1);
	createInfo.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
	createInfo.setImageSharingMode(sharingMode);
	createInfo.setQueueFamilyIndices(queueFamilyIndices);
	createInfo.setPreTransform(support.capabilities.currentTransform);
	createInfo.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque);
	createInfo.setPresentMode(presentMode);
	createInfo.setClipped(true);

	_swapchain = _device.createSwapchainKHR(createInfo);

	std::vector<vk::Image> images = _device.getSwapchainImagesKHR(_swapchain);
	_swapchainImages.resize(images.size());

	// Resources

	uint32_t _width = _swapchainExtent.width;
	uint32_t _height = _swapchainExtent.height;

	vk::PhysicalDeviceMemoryProperties memProperties = _physicalDevice.getMemoryProperties();

	vk::Format colorFormat = vk::Format::eB10G11R11UfloatPack32;
	_color = Attachment::create(_device, _width, _height, colorFormat,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment,
			vk::ImageAspectFlagBits::eColor, memProperties);

	vk::Format depthFormat = vk::Format::eD32Sfloat;
	_depth = Attachment::create(_device, _width, _height, depthFormat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageAspectFlagBits::eDepth,
			memProperties);

	// attachments

	vk::AttachmentDescription finalColorAttachment = {};
	finalColorAttachment.setFormat(surfaceFormat.format);
	finalColorAttachment.setSamples(vk::SampleCountFlagBits::e1);
	finalColorAttachment.setLoadOp(vk::AttachmentLoadOp::eDontCare);
	finalColorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
	finalColorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	finalColorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	finalColorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
	finalColorAttachment.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.setFormat(colorFormat);
	colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
	colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
	colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
	colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
	colorAttachment.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentDescription depthAttachment = {};
	depthAttachment.setFormat(depthFormat);
	depthAttachment.setSamples(vk::SampleCountFlagBits::e1);
	depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
	depthAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
	depthAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	depthAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	depthAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
	depthAttachment.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	// references

	vk::AttachmentReference finalColorRef = {};
	finalColorRef.setAttachment(0);
	finalColorRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference colorRef = {};
	colorRef.setAttachment(1);
	colorRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference colorShaderReadRef = {};
	colorShaderReadRef.setAttachment(1);
	colorShaderReadRef.setLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

	vk::AttachmentReference depthRef = {};
	depthRef.setAttachment(2);
	depthRef.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	// subpasses

	vk::SubpassDescription depthPass = {};
	depthPass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	depthPass.setPDepthStencilAttachment(&depthRef);

	vk::SubpassDescription mainPass = {};
	mainPass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	mainPass.setColorAttachments(colorRef);
	mainPass.setPDepthStencilAttachment(&depthRef);

	vk::SubpassDescription tonemapPass = {};
	tonemapPass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	tonemapPass.setColorAttachments(finalColorRef);
	tonemapPass.setInputAttachments(colorShaderReadRef);

	// dependencies

	vk::SubpassDependency depthDependency = {};
	depthDependency.setSrcSubpass(DEPTH_PASS);
	depthDependency.setDstSubpass(MAIN_PASS);
	depthDependency.setSrcStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests |
									vk::PipelineStageFlagBits::eLateFragmentTests);
	depthDependency.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader);
	depthDependency.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
	depthDependency.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

	vk::SubpassDependency mainDependency = {};
	mainDependency.setSrcSubpass(MAIN_PASS);
	mainDependency.setDstSubpass(TONEMAP_PASS);
	mainDependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
	mainDependency.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader);
	mainDependency.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
	mainDependency.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

	// render pass

	std::array<vk::AttachmentDescription, 3> attachments = {
		finalColorAttachment,
		colorAttachment,
		depthAttachment,
	};

	std::array<vk::SubpassDescription, 3> subpasses = {
		depthPass,
		mainPass,
		tonemapPass,
	};

	std::array<vk::SubpassDependency, 2> dependencies = {
		depthDependency,
		mainDependency,
	};

	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.setAttachments(attachments);
	renderPassInfo.setSubpasses(subpasses);
	renderPassInfo.setDependencies(dependencies);

	_renderPass = _device.createRenderPass(renderPassInfo);

	// framebuffers

	vk::ImageSubresourceRange subresourceRange = {};
	subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
	subresourceRange.setBaseMipLevel(0);
	subresourceRange.setLevelCount(1);
	subresourceRange.setBaseArrayLayer(0);
	subresourceRange.setLayerCount(1);

	for (size_t i = 0; i < images.size(); i++) {
		vk::ImageViewCreateInfo createInfo = {};
		createInfo.setImage(images[i]);
		createInfo.setViewType(vk::ImageViewType::e2D);
		createInfo.setFormat(surfaceFormat.format);
		createInfo.setSubresourceRange(subresourceRange);

		vk::ImageView finalColorView = _device.createImageView(createInfo);

		std::array<vk::ImageView, 3> attachmentViews = {
			finalColorView,
			_color.getImageView(),
			_depth.getImageView(),
		};

		vk::FramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.setRenderPass(_renderPass);
		framebufferInfo.setAttachments(attachmentViews);
		framebufferInfo.setWidth(_swapchainExtent.width);
		framebufferInfo.setHeight(_swapchainExtent.height);
		framebufferInfo.setLayers(1);

		vk::Framebuffer framebuffer;
		vk::Result err = _device.createFramebuffer(&framebufferInfo, nullptr, &framebuffer);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Swapchain framebuffer creation failed!");

		_swapchainImages[i] = { finalColorView, framebuffer };
	}
}

void VulkanContext::_destroySwapchain() {
	_color.destroy(_device);
	_depth.destroy(_device);

	for (uint32_t i = 0; i < _swapchainImages.size(); i++) {
		_device.destroyFramebuffer(_swapchainImages[i].framebuffer, nullptr);
		_device.destroyImageView(_swapchainImages[i].view, nullptr);
	}
	_swapchainImages.clear();

	_device.destroySwapchainKHR(_swapchain, nullptr);
	_device.destroyRenderPass(_renderPass, nullptr);
}

void VulkanContext::initialize(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
	if (_initialized)
		return;

	this->_surface = surface;
	_physicalDevice = pickPhysicalDevice(_instance, surface);
	_device = createDevice(_physicalDevice, surface, _validation);

	QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, surface);
	_graphicsQueue = _device.getQueue(indices.graphicsFamily, 0);
	_presentQueue = _device.getQueue(indices.presentFamily, 0);

	_graphicsQueueFamily = indices.graphicsFamily;

	_createSwapchain(width, height);

	vk::CommandPoolCreateInfo createInfo = {};
	createInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	createInfo.setQueueFamilyIndex(indices.graphicsFamily);

	_commandPool = _device.createCommandPool(createInfo);

	_initialized = true;
}

void VulkanContext::recreateSwapchain(uint32_t width, uint32_t height) {
	_device.waitIdle();

	_destroySwapchain();
	_createSwapchain(width, height);
}

vk::Instance VulkanContext::getInstance() const {
	return _instance;
}

vk::SurfaceKHR VulkanContext::getSurface() const {
	return _surface;
}

vk::PhysicalDevice VulkanContext::getPhysicalDevice() const {
	return _physicalDevice;
}

vk::Device VulkanContext::getDevice() const {
	return _device;
}

vk::Queue VulkanContext::getGraphicsQueue() const {
	return _graphicsQueue;
}

vk::Queue VulkanContext::getPresentQueue() const {
	return _presentQueue;
}

uint32_t VulkanContext::getGraphicsQueueFamily() const {
	return _graphicsQueueFamily;
}

vk::SwapchainKHR VulkanContext::getSwapchain() const {
	return _swapchain;
}

vk::Extent2D VulkanContext::getSwapchainExtent() const {
	return _swapchainExtent;
}

vk::RenderPass VulkanContext::getRenderPass() const {
	return _renderPass;
}

vk::Framebuffer VulkanContext::getFramebuffer(uint32_t imageIndex) const {
	return _swapchainImages[imageIndex].framebuffer;
}

Attachment VulkanContext::getColorAttachment() const {
	return _color;
}

vk::CommandPool VulkanContext::getCommandPool() const {
	return _commandPool;
}

VulkanContext::VulkanContext(std::vector<const char *> extensions, bool validation) {
	if (validation && !checkValidationLayerSupport()) {
		SDL_LogWarn(SDL_LOG_PRIORITY_WARN, "Validation not supported!");
		validation = false;
	}

	this->_validation = validation;
	_instance = createInstance(extensions, validation, &_debugMessenger);
}

VulkanContext::~VulkanContext() {
	if (_initialized) {
		_destroySwapchain();

		_device.destroyCommandPool(_commandPool);
		_device.destroy();

		if (_validation)
			DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);

		_instance.destroySurfaceKHR(_surface);
	}

	_instance.destroy();
}
