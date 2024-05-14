#ifndef VK_TYPES_H
#define VK_TYPES_H

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

struct AllocatedBuffer {
	VmaAllocation allocation;
	vk::Buffer buffer;
	vk::DeviceSize size;

	static AllocatedBuffer create(VmaAllocator allocator, vk::BufferUsageFlags usage,
			vk::DeviceSize size, VmaAllocationInfo *pAllocInfo) {
		VkBufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = static_cast<VkBufferUsageFlags>(usage);

		VmaAllocationCreateInfo allocCreateInfo{};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
								VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VkBuffer buffer;
		VmaAllocation allocation;

		if (pAllocInfo == nullptr) {
			VmaAllocationInfo _allocInfo;
			vmaCreateBuffer(
					allocator, &createInfo, &allocCreateInfo, &buffer, &allocation, &_allocInfo);
		} else {
			vmaCreateBuffer(
					allocator, &createInfo, &allocCreateInfo, &buffer, &allocation, pAllocInfo);
		}

		return { allocation, buffer, size };
	}

	vk::DescriptorBufferInfo getBufferInfo(vk::DeviceSize offset = 0) const {
		return vk::DescriptorBufferInfo(buffer, offset, size);
	}
};

struct AllocatedImage {
	VmaAllocation allocation;
	vk::Image image;

	static AllocatedImage create(VmaAllocator allocator, uint32_t width, uint32_t height,
			vk::Format format, uint32_t mipLevels, vk::ImageUsageFlags usage) {
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.format = static_cast<VkFormat>(format);
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = static_cast<VkImageUsageFlags>(usage);
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		allocCreateInfo.priority = 1.0f;

		VmaAllocation allocation;
		VkImage image;
		vmaCreateImage(allocator, &imageInfo, &allocCreateInfo, &image, &allocation, nullptr);

		return { allocation, image };
	}
};

#endif // !VK_TYPES_H
