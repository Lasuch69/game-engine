#include "render_target.h"
#include "rendering_device.h"
#include <array>

void RenderTarget::create(uint32_t width, uint32_t height, vk::Format format) {
	vk::Device device = RD::getSingleton().getDevice();
	vk::PhysicalDeviceMemoryProperties memProperties = RD::getSingleton().getMemoryProperties();

	_extent = vk::Extent2D(width, height);

	_depth = Attachment::create(device, width, height, vk::Format::eD32Sfloat,
			vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::ImageAspectFlagBits::eDepth,
			memProperties);

	_color = Attachment::create(device, width, height, format,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::ImageAspectFlagBits::eColor, memProperties);

	RD::getSingleton().imageLayoutTransition(_color.getImage(), format, 1, 1,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);

	vk::AttachmentDescription depthAttachment = {};
	depthAttachment.setFormat(_depth->getFormat());
	depthAttachment.setSamples(vk::SampleCountFlagBits::e1);
	depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
	depthAttachment.setStoreOp(vk::AttachmentStoreOp::eDontCare);
	depthAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	depthAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	depthAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
	depthAttachment.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.setFormat(format);
	colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
	colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
	colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
	colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	colorAttachment.setInitialLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	colorAttachment.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference depthReference = {};
	depthReference.setAttachment(0);
	depthReference.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentReference colorReference = {};
	colorReference.setAttachment(1);
	colorReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription depthSubpass = {};
	depthSubpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	depthSubpass.setPDepthStencilAttachment(&depthReference);

	vk::SubpassDescription colorSubpass = {};
	colorSubpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	colorSubpass.setColorAttachments(colorReference);
	colorSubpass.setPDepthStencilAttachment(&depthReference);

	std::array<vk::AttachmentDescription, 2> attachments = {
		depthAttachment,
		colorAttachment,
	};

	std::array<vk::SubpassDescription, 2> subpasses = {
		depthSubpass,
		colorSubpass,
	};

	vk::SubpassDependency dependency = {};
	dependency.setSrcSubpass(0);
	dependency.setDstSubpass(1);
	dependency.setSrcStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests |
							   vk::PipelineStageFlagBits::eLateFragmentTests);
	dependency.setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader);
	dependency.setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite);
	dependency.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.setAttachments(attachments);
	renderPassInfo.setSubpasses(subpasses);
	renderPassInfo.setDependencies(dependency);

	_renderPass = device.createRenderPass(renderPassInfo);

	std::array<vk::ImageView, 2> framebufferAttachments = {
		_depth->getImageView(),
		_color.getImageView(),
	};

	vk::FramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.setRenderPass(_renderPass);
	framebufferInfo.setAttachments(framebufferAttachments);
	framebufferInfo.setWidth(width);
	framebufferInfo.setHeight(height);
	framebufferInfo.setLayers(1);

	vk::Result err = device.createFramebuffer(&framebufferInfo, nullptr, &_framebuffer);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Framebuffer creation failed!");
}

void RenderTarget::createMultiview(uint32_t size, vk::Format format) {
	vk::Device device = RD::getSingleton().getDevice();
	vk::PhysicalDeviceMemoryProperties memProperties = RD::getSingleton().getMemoryProperties();

	_extent = vk::Extent2D(size, size);

	_color = Attachment::create(device, size, size, format,
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
			vk::ImageAspectFlagBits::eColor, memProperties, 6, vk::ImageViewType::eCube,
			vk::ImageCreateFlagBits::eCubeCompatible);

	vk::AttachmentDescription colorAttachment = {};
	colorAttachment.setFormat(format);
	colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
	colorAttachment.setLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
	colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	colorAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
	colorAttachment.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference colorReference = {};
	colorReference.setAttachment(0);
	colorReference.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpass = {};
	subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpass.setColorAttachments(colorReference);

	// write from layer 0 (last bit) to layer 5
	uint32_t viewMask = 0b00111111;
	uint32_t correlationMask = 0b00111111;

	vk::RenderPassMultiviewCreateInfo multiviewInfo = {};
	multiviewInfo.setSubpassCount(1);
	multiviewInfo.setViewMasks(viewMask);
	multiviewInfo.setCorrelationMasks(correlationMask);

	vk::RenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.setAttachments(colorAttachment);
	renderPassInfo.setSubpasses(subpass);
	renderPassInfo.setPNext(&multiviewInfo);

	_renderPass = device.createRenderPass(renderPassInfo);

	vk::ImageView colorView = _color.getImageView();

	vk::FramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.setRenderPass(_renderPass);
	framebufferInfo.setAttachments(colorView);
	framebufferInfo.setWidth(size);
	framebufferInfo.setHeight(size);
	framebufferInfo.setLayers(1);

	vk::Result err = device.createFramebuffer(&framebufferInfo, nullptr, &_framebuffer);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Multiview framebuffer creation failed!");
}

void RenderTarget::destroy() {
	vk::Device device = RD::getSingleton().getDevice();

	_color.destroy(device);

	if (_depth.has_value())
		_depth->destroy(device);

	device.destroyFramebuffer(_framebuffer, nullptr);
	device.destroyRenderPass(_renderPass, nullptr);
}

vk::Framebuffer RenderTarget::getFramebuffer() const {
	return _framebuffer;
}

vk::RenderPass RenderTarget::getRenderPass() const {
	return _renderPass;
}

vk::Extent2D RenderTarget::getExtent() const {
	return _extent;
}

Attachment RenderTarget::getColor() const {
	return _color;
}
