#ifndef ATTACHMENT_H
#define ATTACHMENT_H

#include <cstdint>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

class Attachment {
private:
	vk::Image _image = {};
	vk::ImageView _imageView = {};
	vk::DeviceMemory _imageMemory = {};
	vk::Format _format = vk::Format::eUndefined;

	static uint32_t _findMemoryType(uint32_t typeFilter,
			vk::PhysicalDeviceMemoryProperties memProperties, vk::MemoryPropertyFlags properties) {
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) &&
					(memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("Could not find suitable memory type!");
	}

	static vk::Image _createImage(vk::Device device, uint32_t width, uint32_t height,
			vk::Format format, vk::ImageUsageFlags usage,
			vk::PhysicalDeviceMemoryProperties memProperties, vk::DeviceMemory *pMemory) {
		vk::ImageCreateInfo createInfo = {};
		createInfo.setImageType(vk::ImageType::e2D);
		createInfo.setExtent(vk::Extent3D(width, height, 1));
		createInfo.setMipLevels(1);
		createInfo.setArrayLayers(1);
		createInfo.setFormat(format);
		createInfo.setTiling(vk::ImageTiling::eOptimal);
		createInfo.setInitialLayout(vk::ImageLayout::eUndefined);
		createInfo.setUsage(usage);
		createInfo.setSamples(vk::SampleCountFlagBits::e1);
		createInfo.setSharingMode(vk::SharingMode::eExclusive);

		vk::Image image = device.createImage(createInfo);

		vk::MemoryRequirements memRequirements = device.getImageMemoryRequirements(image);

		uint32_t memoryTypeIndex = _findMemoryType(memRequirements.memoryTypeBits, memProperties,
				vk::MemoryPropertyFlagBits::eDeviceLocal);

		vk::MemoryAllocateInfo allocInfo = {};
		allocInfo.setAllocationSize(memRequirements.size);
		allocInfo.setMemoryTypeIndex(memoryTypeIndex);

		vk::Result err = device.allocateMemory(&allocInfo, nullptr, pMemory);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Attachment image memory allocation failed!");

		device.bindImageMemory(image, *pMemory, 0);
		return image;
	}

	static vk::ImageView _createView(vk::Device device, vk::Image image, vk::Format format,
			vk::ImageAspectFlagBits aspectFlags) {
		vk::ImageSubresourceRange subresourceRange = {};
		subresourceRange.setAspectMask(aspectFlags);
		subresourceRange.setBaseMipLevel(0);
		subresourceRange.setLevelCount(1);
		subresourceRange.setBaseArrayLayer(0);
		subresourceRange.setLayerCount(1);

		vk::ImageViewCreateInfo createInfo = {};
		createInfo.setImage(image);
		createInfo.setViewType(vk::ImageViewType::e2D);
		createInfo.setFormat(format);
		createInfo.setSubresourceRange(subresourceRange);

		return device.createImageView(createInfo);
	}

public:
	static Attachment create(vk::Device device, uint32_t width, uint32_t height, vk::Format format,
			vk::ImageUsageFlags usage, vk::ImageAspectFlagBits aspectFlags,
			vk::PhysicalDeviceMemoryProperties memProperties) {
		vk::DeviceMemory memory;
		vk::Image image =
				_createImage(device, width, height, format, usage, memProperties, &memory);
		vk::ImageView view = _createView(device, image, format, aspectFlags);

		return Attachment(image, view, memory, format);
	}

	void destroy(vk::Device device) {
		device.destroyImageView(_imageView, nullptr);
		device.destroyImage(_image, nullptr);
		device.freeMemory(_imageMemory, nullptr);
	}

	vk::Image getImage() const {
		return _image;
	}

	vk::ImageView getImageView() const {
		return _imageView;
	}

	vk::Format getFormat() const {
		return _format;
	}

	Attachment() {}

	Attachment(vk::Image image, vk::ImageView view, vk::DeviceMemory memory, vk::Format format) {
		_image = image;
		_imageView = view;
		_imageMemory = memory;
		_format = format;
	}
};

#endif // !ATTACHMENT_H
