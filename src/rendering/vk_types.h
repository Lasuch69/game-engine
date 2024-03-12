#ifndef VK_TYPES_H
#define VK_TYPES_H

#include <cstdint>
#include <optional>
#include <vector>

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

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

struct AllocatedBuffer {
	VmaAllocation allocation;
	vk::Buffer buffer;
};

#endif // !VK_TYPES_H
