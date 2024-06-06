#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include <cstdint>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_structs.hpp>

#include <rendering/types/allocated.h>

class RenderTarget {
	vk::Framebuffer _framebuffer;
	vk::Extent2D _extent;
	vk::RenderPass _renderPass;

	AllocatedImage _colorImage;
	vk::ImageView _colorView;

	RenderTarget(vk::Framebuffer framebuffer, vk::Extent2D extent, vk::RenderPass renderPass,
			AllocatedImage colorImage, vk::ImageView colorView);

public:
	static RenderTarget create(vk::Device device, uint32_t width, uint32_t height,
			vk::Format format, AllocatedImage colorImage, vk::ImageView colorView);
	static RenderTarget createCubemap(vk::Device device, uint32_t size, vk::Format format,
			AllocatedImage colorImage, vk::ImageView colorView);
	void destroy(vk::Device device);

	inline vk::Framebuffer getFramebuffer() const {
		return _framebuffer;
	}

	inline vk::Extent2D getExtent() const {
		return _extent;
	}

	inline vk::RenderPass getRenderPass() const {
		return _renderPass;
	}

	inline AllocatedImage getColorImage() const {
		return _colorImage;
	}

	inline vk::ImageView getColorView() const {
		return _colorView;
	}
};

#endif // !RENDER_TARGET_H
