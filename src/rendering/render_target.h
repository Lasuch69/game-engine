#ifndef RENDER_TARGET_H
#define RENDER_TARGET_H

#include <cstdint>
#include <optional>

#include <vulkan/vulkan.hpp>

#include "types/attachment.h"

class RenderTarget {
private:
	vk::Framebuffer _framebuffer;
	vk::RenderPass _renderPass;
	vk::Extent2D _extent;

	std::optional<Attachment> _depth;
	Attachment _color;

public:
	void create(uint32_t width, uint32_t height, vk::Format format);
	void createMultiview(uint32_t size, vk::Format format);
	void destroy();

	vk::Framebuffer getFramebuffer() const;
	vk::RenderPass getRenderPass() const;
	vk::Extent2D getExtent() const;

	Attachment getColor() const;
};

#endif // !RENDER_TARGET_H
