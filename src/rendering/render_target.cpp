#include <rendering/types/allocated.h>

#include "render_target.h"

RenderTarget::RenderTarget(vk::Framebuffer framebuffer, vk::Extent2D extent,
		vk::RenderPass renderPass, AllocatedImage colorImage, vk::ImageView colorView) {
	_framebuffer = framebuffer;
	_extent = extent;
	_renderPass = renderPass;

	_colorImage = colorImage;
	_colorView = colorView;
}

RenderTarget RenderTarget::create(vk::Device device, uint32_t width, uint32_t height,
		vk::Format format, AllocatedImage colorImage, vk::ImageView colorView) {
	vk::AttachmentDescription colorAttachmentDescription = {};
	colorAttachmentDescription.setFormat(format);
	colorAttachmentDescription.setSamples(vk::SampleCountFlagBits::e1);
	colorAttachmentDescription.setLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachmentDescription.setStoreOp(vk::AttachmentStoreOp::eStore);
	colorAttachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	colorAttachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
	colorAttachmentDescription.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.setAttachment(0);
	colorAttachmentReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpass = {};
	subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpass.setColorAttachments(colorAttachmentReference);

	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.setAttachments(colorAttachmentDescription);
	renderPassInfo.setSubpasses(subpass);

	vk::RenderPass renderPass = device.createRenderPass(renderPassInfo);

	vk::FramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.setRenderPass(renderPass);
	framebufferInfo.setAttachments(colorView);
	framebufferInfo.setWidth(width);
	framebufferInfo.setHeight(height);
	framebufferInfo.setLayers(1);

	vk::Framebuffer framebuffer;
	vk::Result err = device.createFramebuffer(&framebufferInfo, nullptr, &framebuffer);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Failed to create multiview framebuffer!");

	return RenderTarget(
			framebuffer, vk::Extent2D(width, height), renderPass, colorImage, colorView);
}

RenderTarget RenderTarget::createCubemap(vk::Device device, uint32_t size, vk::Format format,
		AllocatedImage colorImage, vk::ImageView colorView) {
	vk::AttachmentDescription colorAttachmentDescription = {};
	colorAttachmentDescription.setFormat(format);
	colorAttachmentDescription.setSamples(vk::SampleCountFlagBits::e1);
	colorAttachmentDescription.setLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachmentDescription.setStoreOp(vk::AttachmentStoreOp::eStore);
	colorAttachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	colorAttachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
	colorAttachmentDescription.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.setAttachment(0);
	colorAttachmentReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpass = {};
	subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpass.setColorAttachments(colorAttachmentReference);

	// write from layer 0 (last bit) to layer 5
	uint32_t viewMask = 0b00111111;
	uint32_t correlationMask = 0b00111111;

	vk::RenderPassMultiviewCreateInfo multiviewCreateInfo = {};
	multiviewCreateInfo.setSubpassCount(1);
	multiviewCreateInfo.setViewMasks(viewMask);
	multiviewCreateInfo.setCorrelationMasks(correlationMask);

	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.setAttachments(colorAttachmentDescription);
	renderPassInfo.setSubpasses(subpass);
	renderPassInfo.setPNext(&multiviewCreateInfo);

	vk::RenderPass renderPass = device.createRenderPass(renderPassInfo);

	vk::FramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.setRenderPass(renderPass);
	framebufferInfo.setAttachments(colorView);
	framebufferInfo.setWidth(size);
	framebufferInfo.setHeight(size);
	framebufferInfo.setLayers(1);

	vk::Framebuffer framebuffer;
	vk::Result err = device.createFramebuffer(&framebufferInfo, nullptr, &framebuffer);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Failed to create multiview framebuffer!");

	return RenderTarget(framebuffer, vk::Extent2D(size, size), renderPass, colorImage, colorView);
}

void RenderTarget::destroy(vk::Device device) {
	device.destroyFramebuffer(_framebuffer, nullptr);
	device.destroyRenderPass(_renderPass, nullptr);
}
