#ifndef VK_TYPES_H
#define VK_TYPES_H

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

struct AllocatedBuffer {
	VmaAllocation allocation;
	vk::Buffer buffer;
};

struct AllocatedImage {
	VmaAllocation allocation;
	vk::Image image;
};

#endif // !VK_TYPES_H
