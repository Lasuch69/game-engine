#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_enums.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <rendering/render_target.h>
#include <rendering/rendering_device.h>

#include "environment.h"

void Environment::create(RenderingDevice &renderingDevice) {
	vk::Device device = renderingDevice.getDevice();
	vk::DescriptorPool descriptorPool = renderingDevice.getDescriptorPool();

	vk::ImageUsageFlags usage =
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;

	AllocatedImage image =
			renderingDevice.imageCreate(512, 512, vk::Format::eR16G16Sfloat, 1, usage);

	vk::ImageView view = renderingDevice.imageViewCreate(image.image, vk::Format::eR16G16Sfloat, 1);

	RenderTarget brdfRT =
			RenderTarget::create(device, 512, 512, vk::Format::eR16G16Sfloat, image, view);

	RenderTarget filterRT =
			RenderTarget::createCubemap(device, 1, vk::Format::eR16G16B16A16Sfloat, image, view);

	_brdfEffect.create(device, descriptorPool, brdfRT.getRenderPass());
	_irradianceFilterEffect.create(device, descriptorPool, filterRT.getRenderPass());
	_specularFilterEffect.create(device, descriptorPool, filterRT.getRenderPass());
}
