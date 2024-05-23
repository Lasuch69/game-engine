#include <cstdint>
#include <stdexcept>

#include "shaders/brdf.gen.h"
#include "shaders/cubemap.gen.h"
#include "shaders/irradiance_filter.gen.h"
#include "shaders/specular_filter.gen.h"

#include "../rendering_device.h"
#include "../types/attachment.h"

#include "environment_effects.h"

static vk::ShaderModule createModule(vk::Device device, const uint32_t *pCode, size_t size) {
	vk::ShaderModuleCreateInfo createInfo = {};
	createInfo.setPCode(pCode);
	createInfo.setCodeSize(size);

	return device.createShaderModule(createInfo);
}

static vk::Pipeline createPipeline(vk::Device device, vk::ShaderModule vertexStage,
		vk::ShaderModule fragmentStage, vk::PipelineLayout pipelineLayout,
		vk::RenderPass renderPass) {
	vk::PipelineShaderStageCreateInfo vertexStageInfo;
	vertexStageInfo.setModule(vertexStage);
	vertexStageInfo.setStage(vk::ShaderStageFlagBits::eVertex);
	vertexStageInfo.setPName("main");

	vk::PipelineShaderStageCreateInfo fragmentStageInfo;
	fragmentStageInfo.setModule(fragmentStage);
	fragmentStageInfo.setStage(vk::ShaderStageFlagBits::eFragment);
	fragmentStageInfo.setPName("main");

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertexStageInfo, fragmentStageInfo };

	vk::PipelineVertexInputStateCreateInfo vertexInput = {};

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.setViewportCount(1);
	viewportState.setScissorCount(1);

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.setDepthBiasEnable(VK_FALSE);
	rasterizer.setRasterizerDiscardEnable(VK_FALSE);
	rasterizer.setPolygonMode(vk::PolygonMode::eFill);
	rasterizer.setLineWidth(1.0f);
	rasterizer.setCullMode(vk::CullModeFlagBits::eBack);
	rasterizer.setFrontFace(vk::FrontFace::eCounterClockwise);
	rasterizer.setDepthBiasEnable(VK_FALSE);

	vk::PipelineMultisampleStateCreateInfo multisampling;
	multisampling.setSampleShadingEnable(VK_FALSE);
	multisampling.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	vk::PipelineDepthStencilStateCreateInfo depthStencil;
	depthStencil.setDepthTestEnable(VK_FALSE);
	depthStencil.setDepthWriteEnable(VK_FALSE);
	depthStencil.setDepthBoundsTestEnable(VK_FALSE);
	depthStencil.setStencilTestEnable(VK_FALSE);

	vk::ColorComponentFlags colorWriteMask =
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.setColorWriteMask(colorWriteMask);
	colorBlendAttachment.setBlendEnable(VK_FALSE);

	vk::PipelineColorBlendStateCreateInfo colorBlending;
	colorBlending.setLogicOpEnable(VK_FALSE);
	colorBlending.setLogicOp(vk::LogicOp::eCopy);
	colorBlending.setAttachments(colorBlendAttachment);
	colorBlending.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

	std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};

	vk::PipelineDynamicStateCreateInfo dynamicState;
	dynamicState.setDynamicStates(dynamicStates);

	vk::GraphicsPipelineCreateInfo createInfo;
	createInfo.setStages(shaderStages);
	createInfo.setPVertexInputState(&vertexInput);
	createInfo.setPInputAssemblyState(&inputAssembly);
	createInfo.setPViewportState(&viewportState);
	createInfo.setPRasterizationState(&rasterizer);
	createInfo.setPMultisampleState(&multisampling);
	createInfo.setPDepthStencilState(&depthStencil);
	createInfo.setPColorBlendState(&colorBlending);
	createInfo.setPDynamicState(&dynamicState);
	createInfo.setLayout(pipelineLayout);
	createInfo.setRenderPass(renderPass);
	createInfo.setSubpass(0);

	vk::ResultValue<vk::Pipeline> result = device.createGraphicsPipeline(nullptr, createInfo);

	if (result.result != vk::Result::eSuccess)
		throw std::runtime_error("Graphics pipeline creation failed!");

	return result.value;
}

static vk::Sampler createSampler(vk::Device device, uint32_t mipLevels) {
	float maxLod = static_cast<float>(mipLevels);

	vk::SamplerCreateInfo createInfo = {};
	createInfo.setMagFilter(vk::Filter::eLinear);
	createInfo.setMinFilter(vk::Filter::eLinear);
	createInfo.setAddressModeU(vk::SamplerAddressMode::eClampToEdge);
	createInfo.setAddressModeV(vk::SamplerAddressMode::eClampToEdge);
	createInfo.setAddressModeW(vk::SamplerAddressMode::eClampToEdge);
	createInfo.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
	createInfo.setUnnormalizedCoordinates(false);
	createInfo.setCompareEnable(false);
	createInfo.setCompareOp(vk::CompareOp::eAlways);
	createInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
	createInfo.setMinLod(0.0f);
	createInfo.setMaxLod(maxLod);
	createInfo.setMipLodBias(0.0f);

	return device.createSampler(createInfo);
}

RenderTarget RenderTarget::create(
		vk::Device device, uint32_t size, vk::PhysicalDeviceMemoryProperties memProperties) {
	vk::Format format = vk::Format::eR32G32B32A32Sfloat;
	vk::ImageUsageFlags usage =
			vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;

	vk::ImageAspectFlagBits aspectFlags = vk::ImageAspectFlagBits::eColor;

	assert(size > 0);

	uint32_t arrayLayers = 6;

	Attachment color = Attachment::create(device, size, size, format, usage, aspectFlags,
			memProperties, arrayLayers, vk::ImageViewType::eCube,
			vk::ImageCreateFlagBits::eCubeCompatible);

	vk::AttachmentDescription colorAttachmentDescription = {};
	colorAttachmentDescription.setFormat(format);
	colorAttachmentDescription.setSamples(vk::SampleCountFlagBits::e1);
	colorAttachmentDescription.setLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachmentDescription.setStoreOp(vk::AttachmentStoreOp::eStore);
	colorAttachmentDescription.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
	colorAttachmentDescription.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
	colorAttachmentDescription.setInitialLayout(vk::ImageLayout::eUndefined);
	colorAttachmentDescription.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::AttachmentReference colorRef = {};
	colorRef.setAttachment(0);
	colorRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpass = {};
	subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
	subpass.setColorAttachments(colorRef);

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

	vk::ImageView colorView = color.getImageView();

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

	return RenderTarget(framebuffer, renderPass, color, size);
}

vk::Framebuffer RenderTarget::getFramebuffer() const {
	return _framebuffer;
}

vk::RenderPass RenderTarget::getRenderPass() const {
	return _renderPass;
}

Attachment RenderTarget::getColorAttachment() const {
	return _color;
}

uint32_t RenderTarget::getSize() const {
	return _size;
}

void RenderTarget::destroy(vk::Device device) {
	_color.destroy(device);

	device.destroyFramebuffer(_framebuffer, nullptr);
	device.destroyRenderPass(_renderPass, nullptr);
}

RenderTarget::RenderTarget(
		vk::Framebuffer framebuffer, vk::RenderPass renderPass, Attachment color, uint32_t size) {
	_framebuffer = framebuffer;
	_renderPass = renderPass;
	_color = color;
	_size = size;
}

void EnvironmentEffects::_createDescriptors(vk::DescriptorPool descriptorPool) {
	// brdf

	{
		vk::DescriptorSetLayoutBinding binding = {};
		binding.setBinding(0);
		binding.setDescriptorType(vk::DescriptorType::eStorageImage);
		binding.setDescriptorCount(1);
		binding.setStageFlags(vk::ShaderStageFlagBits::eCompute);

		vk::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.setBindings(binding);

		vk::Result err = _device.createDescriptorSetLayout(&createInfo, nullptr, &_brdfSetLayout);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create BRDF set layout!");

		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.setDescriptorPool(descriptorPool);
		allocInfo.setDescriptorSetCount(1);
		allocInfo.setSetLayouts(_brdfSetLayout);

		err = _device.allocateDescriptorSets(&allocInfo, &_brdfSet);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Failed to allocate BRDF set!");
	}

	// cubemap

	{
		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {};
		bindings[0].setBinding(0);
		bindings[0].setDescriptorType(vk::DescriptorType::eStorageImage);
		bindings[0].setDescriptorCount(1);
		bindings[0].setStageFlags(vk::ShaderStageFlagBits::eCompute);

		bindings[1].setBinding(1);
		bindings[1].setDescriptorType(vk::DescriptorType::eStorageImage);
		bindings[1].setDescriptorCount(1);
		bindings[1].setStageFlags(vk::ShaderStageFlagBits::eCompute);

		vk::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.setBindings(bindings);

		vk::Result err =
				_device.createDescriptorSetLayout(&createInfo, nullptr, &_cubemapSetLayout);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create cubemap set layout!");

		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.setDescriptorPool(descriptorPool);
		allocInfo.setDescriptorSetCount(1);
		allocInfo.setSetLayouts(_cubemapSetLayout);

		err = _device.allocateDescriptorSets(&allocInfo, &_cubemapSet);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Failed to allocate cubemap set!");
	}

	// filter

	{
		vk::DescriptorSetLayoutBinding binding = {};
		binding.setBinding(0);
		binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
		binding.setDescriptorCount(1);
		binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.setBindings(binding);

		vk::Result err = _device.createDescriptorSetLayout(&createInfo, nullptr, &_filterSetLayout);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create filter set layout!");

		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.setDescriptorPool(descriptorPool);
		allocInfo.setDescriptorSetCount(1);
		allocInfo.setSetLayouts(_filterSetLayout);

		err = _device.allocateDescriptorSets(&allocInfo, &_filterSet);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Failed to allocate filter set!");
	}
}

void EnvironmentEffects::_createPipelines() {
	{
		vk::PipelineLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.setSetLayouts(_brdfSetLayout);

		_brdfPipelineLayout = _device.createPipelineLayout(layoutCreateInfo);

		BrdfShader shader;

		uint32_t codeSize = sizeof(shader.computeCode);
		vk::ShaderModule computeModule = createModule(_device, shader.computeCode, codeSize);

		vk::PipelineShaderStageCreateInfo computeStageInfo = {};
		computeStageInfo.setModule(computeModule);
		computeStageInfo.setStage(vk::ShaderStageFlagBits::eCompute);
		computeStageInfo.setPName("main");

		vk::ComputePipelineCreateInfo createInfo = {};
		createInfo.setStage(computeStageInfo);
		createInfo.setLayout(_brdfPipelineLayout);

		vk::ResultValue<vk::Pipeline> result = _device.createComputePipeline({}, createInfo);

		if (result.result != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create BRDF compute pipeline!");

		_brdfPipeline = result.value;

		_device.destroyShaderModule(computeModule);
	}

	{
		vk::PipelineLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.setSetLayouts(_cubemapSetLayout);

		_cubemapPipelineLayout = _device.createPipelineLayout(layoutCreateInfo);

		CubemapShader shader;

		uint32_t codeSize = sizeof(shader.computeCode);
		vk::ShaderModule computeModule = createModule(_device, shader.computeCode, codeSize);

		vk::PipelineShaderStageCreateInfo computeStageInfo = {};
		computeStageInfo.setModule(computeModule);
		computeStageInfo.setStage(vk::ShaderStageFlagBits::eCompute);
		computeStageInfo.setPName("main");

		vk::ComputePipelineCreateInfo createInfo = {};
		createInfo.setStage(computeStageInfo);
		createInfo.setLayout(_cubemapPipelineLayout);

		vk::ResultValue<vk::Pipeline> result = _device.createComputePipeline({}, createInfo);

		if (result.result != vk::Result::eSuccess)
			throw std::runtime_error("Failed to create cubemap compute pipeline!");

		_cubemapPipeline = result.value;

		_device.destroyShaderModule(computeModule);
	}

	RenderTarget rt = RenderTarget::create(_device, 1, _memProperties);

	{
		vk::PipelineLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.setSetLayouts(_filterSetLayout);

		_irradiancePipelineLayout = _device.createPipelineLayout(layoutCreateInfo);

		IrradianceFilterShader shader;

		uint32_t codeSize = sizeof(shader.vertexCode);
		vk::ShaderModule vertexStage = createModule(_device, shader.vertexCode, codeSize);

		codeSize = sizeof(shader.fragmentCode);
		vk::ShaderModule fragmentStage = createModule(_device, shader.fragmentCode, codeSize);

		_irradiancePipeline = createPipeline(
				_device, vertexStage, fragmentStage, _irradiancePipelineLayout, rt.getRenderPass());

		_device.destroyShaderModule(vertexStage);
		_device.destroyShaderModule(fragmentStage);
	}

	{
		vk::PushConstantRange pushConstants;
		pushConstants.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		pushConstants.setOffset(0);
		pushConstants.setSize(sizeof(SpecularFilterConstants));

		vk::PipelineLayoutCreateInfo layoutCreateInfo = {};
		layoutCreateInfo.setPushConstantRanges(pushConstants);
		layoutCreateInfo.setSetLayouts(_filterSetLayout);

		_specularPipelineLayout = _device.createPipelineLayout(layoutCreateInfo);

		SpecularFilterShader shader;

		uint32_t codeSize = sizeof(shader.vertexCode);
		vk::ShaderModule vertexStage = createModule(_device, shader.vertexCode, codeSize);

		codeSize = sizeof(shader.fragmentCode);
		vk::ShaderModule fragmentStage = createModule(_device, shader.fragmentCode, codeSize);

		_specularPipeline = createPipeline(
				_device, vertexStage, fragmentStage, _specularPipelineLayout, rt.getRenderPass());

		_device.destroyShaderModule(vertexStage);
		_device.destroyShaderModule(fragmentStage);
	}

	rt.destroy(_device);
}

void EnvironmentEffects::_updateBrdfSet(vk::ImageView dstImageView) {
	vk::DescriptorImageInfo imageInfo = {};
	imageInfo.setImageView(dstImageView);
	imageInfo.setImageLayout(vk::ImageLayout::eGeneral);

	vk::WriteDescriptorSet writeInfo = {};
	writeInfo.setDstSet(_brdfSet);
	writeInfo.setDstBinding(0);
	writeInfo.setDescriptorType(vk::DescriptorType::eStorageImage);
	writeInfo.setDescriptorCount(1);
	writeInfo.setImageInfo(imageInfo);

	_device.updateDescriptorSets(writeInfo, nullptr);
}

void EnvironmentEffects::_updateCubemapSet(
		vk::ImageView srcImageView, vk::ImageView dstCubemapView) {
	std::array<vk::DescriptorImageInfo, 2> imageInfos = {};

	imageInfos[0].setImageView(srcImageView);
	imageInfos[0].setImageLayout(vk::ImageLayout::eGeneral);

	imageInfos[1].setImageView(dstCubemapView);
	imageInfos[1].setImageLayout(vk::ImageLayout::eGeneral);

	std::array<vk::WriteDescriptorSet, 2> writeInfos = {};

	writeInfos[0].setDstSet(_cubemapSet);
	writeInfos[0].setDstBinding(0);
	writeInfos[0].setDescriptorType(vk::DescriptorType::eStorageImage);
	writeInfos[0].setDescriptorCount(1);
	writeInfos[0].setImageInfo(imageInfos[0]);

	writeInfos[1].setDstSet(_cubemapSet);
	writeInfos[1].setDstBinding(1);
	writeInfos[1].setDescriptorType(vk::DescriptorType::eStorageImage);
	writeInfos[1].setDescriptorCount(1);
	writeInfos[1].setImageInfo(imageInfos[1]);

	_device.updateDescriptorSets(writeInfos, nullptr);
}

void EnvironmentEffects::_updateFilterSet(vk::ImageView srcImageView, vk::Sampler sampler) {
	vk::DescriptorImageInfo imageInfo = {};
	imageInfo.setImageView(srcImageView);
	imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfo.setSampler(sampler);

	vk::WriteDescriptorSet writeInfo = {};
	writeInfo.setDstSet(_filterSet);
	writeInfo.setDstBinding(0);
	writeInfo.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeInfo.setDescriptorCount(1);
	writeInfo.setImageInfo(imageInfo);

	_device.updateDescriptorSets(writeInfo, nullptr);
}

void EnvironmentEffects::_drawIrradianceFilter(
		vk::CommandBuffer commandBuffer, RenderTarget renderTarget) {
	vk::Viewport viewport = {};
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setWidth(renderTarget.getSize());
	viewport.setHeight(renderTarget.getSize());
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Extent2D extent(renderTarget.getSize(), renderTarget.getSize());
	vk::Offset2D offset(0, 0);

	vk::Rect2D scissor = {};
	scissor.setExtent(extent);
	scissor.setOffset(offset);

	vk::Rect2D renderArea = {};
	renderArea.setExtent(extent);
	renderArea.setOffset(offset);

	vk::RenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.setRenderPass(renderTarget.getRenderPass());
	renderPassBeginInfo.setFramebuffer(renderTarget.getFramebuffer());
	renderPassBeginInfo.setRenderArea(renderArea);

	commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);

	commandBuffer.setViewport(0, viewport);
	commandBuffer.setScissor(0, scissor);

	vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics;
	commandBuffer.bindPipeline(bindPoint, _irradiancePipeline);
	commandBuffer.bindDescriptorSets(bindPoint, _irradiancePipelineLayout, 0, _filterSet, nullptr);

	commandBuffer.draw(3, 1, 0, 0);

	commandBuffer.endRenderPass();
}

void EnvironmentEffects::_drawSpecularFilter(vk::CommandBuffer commandBuffer,
		RenderTarget renderTarget, uint32_t size, float roughness) {
	vk::Viewport viewport = {};
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setWidth(renderTarget.getSize());
	viewport.setHeight(renderTarget.getSize());
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Extent2D extent(renderTarget.getSize(), renderTarget.getSize());
	vk::Offset2D offset(0, 0);

	vk::Rect2D scissor = {};
	scissor.setExtent(extent);
	scissor.setOffset(offset);

	vk::Rect2D renderArea = {};
	renderArea.setExtent(extent);
	renderArea.setOffset(offset);

	vk::RenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.setRenderPass(renderTarget.getRenderPass());
	renderPassBeginInfo.setFramebuffer(renderTarget.getFramebuffer());
	renderPassBeginInfo.setRenderArea(renderArea);

	commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);

	commandBuffer.setViewport(0, viewport);
	commandBuffer.setScissor(0, scissor);

	vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics;
	commandBuffer.bindPipeline(bindPoint, _specularPipeline);
	commandBuffer.bindDescriptorSets(bindPoint, _specularPipelineLayout, 0, _filterSet, nullptr);

	SpecularFilterConstants constants = {};
	constants.size = size;
	constants.roughness = roughness;

	commandBuffer.pushConstants(_specularPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0,
			sizeof(constants), &constants);

	commandBuffer.draw(3, 1, 0, 0);

	commandBuffer.endRenderPass();
}

void EnvironmentEffects::_copyImageToLevel(
		vk::Image srcImage, vk::Image dstImage, uint32_t level, uint32_t size) {
	vk::ImageSubresourceLayers srcSubresource = {};
	srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
	srcSubresource.setMipLevel(0);
	srcSubresource.setBaseArrayLayer(0);
	srcSubresource.setLayerCount(6);

	vk::ImageSubresourceLayers dstSubresource = {};
	dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
	dstSubresource.setMipLevel(level);
	dstSubresource.setBaseArrayLayer(0);
	dstSubresource.setLayerCount(6);

	vk::Extent3D extent(size, size, 1);

	vk::ImageCopy copyInfo = {};
	copyInfo.setSrcSubresource(srcSubresource);
	copyInfo.setDstSubresource(dstSubresource);
	copyInfo.setExtent(extent);

	{
		vk::CommandBuffer commandBuffer = RD::getSingleton().beginSingleTimeCommands();

		// clone framebuffer image level 0 to level i of filteredCube
		commandBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, dstImage,
				vk::ImageLayout::eTransferDstOptimal, copyInfo);

		RD::getSingleton().endSingleTimeCommands(commandBuffer);
	}
}

void EnvironmentEffects::generateBrdf(vk::ImageView dstImageView, uint32_t size) {
	RD &rd = RD::getSingleton();
	_updateBrdfSet(dstImageView);

	vk::CommandBuffer commandBuffer = rd.beginSingleTimeCommands();

	uint32_t groupCount = (size + 15) / 16;
	vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eCompute;

	commandBuffer.bindPipeline(bindPoint, _brdfPipeline);
	commandBuffer.bindDescriptorSets(bindPoint, _brdfPipelineLayout, 0, _brdfSet, nullptr);
	commandBuffer.dispatch(groupCount, groupCount, 1);

	rd.endSingleTimeCommands(commandBuffer);
}

void EnvironmentEffects::imageCopyToCube(
		vk::ImageView srcImageView, vk::ImageView dstCubemapView, uint32_t size) {
	RD &rd = RD::getSingleton();
	_updateCubemapSet(srcImageView, dstCubemapView);

	vk::CommandBuffer commandBuffer = rd.beginSingleTimeCommands();

	uint32_t groupCount = (size + 15) / 16;
	vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eCompute;

	commandBuffer.bindPipeline(bindPoint, _cubemapPipeline);
	commandBuffer.bindDescriptorSets(bindPoint, _cubemapPipelineLayout, 0, _cubemapSet, nullptr);
	commandBuffer.dispatch(groupCount, groupCount, 6);

	rd.endSingleTimeCommands(commandBuffer);
}

void EnvironmentEffects::filterIrradiance(
		vk::ImageView srcCubemapView, vk::Image dstCubemap, vk::Format format, uint32_t size) {
	assert(format == vk::Format::eR32G32B32A32Sfloat);

	vk::Sampler sampler = createSampler(_device, 1);
	_updateFilterSet(srcCubemapView, sampler);

	RenderTarget rt = RenderTarget::create(_device, size, _memProperties);

	RD &rd = RD::getSingleton();

	{
		vk::CommandBuffer commandBuffer = rd.beginSingleTimeCommands();
		_drawIrradianceFilter(commandBuffer, rt);
		rd.endSingleTimeCommands(commandBuffer);
	}

	vk::Image framebufferImage = rt.getColorAttachment().getImage();

	// make framebuffer image valid source
	rd.imageLayoutTransition(framebufferImage, format, 1, 6,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal);

	vk::ImageSubresourceLayers subresource = {};
	subresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
	subresource.setMipLevel(0);
	subresource.setBaseArrayLayer(0);
	subresource.setLayerCount(6);

	vk::ImageCopy copyInfo = {};
	copyInfo.setSrcSubresource(subresource);
	copyInfo.setDstSubresource(subresource);
	copyInfo.setExtent(vk::Extent3D(size, size, 1));

	{
		vk::CommandBuffer commandBuffer = rd.beginSingleTimeCommands();

		commandBuffer.copyImage(framebufferImage, vk::ImageLayout::eTransferSrcOptimal, dstCubemap,
				vk::ImageLayout::eTransferDstOptimal, copyInfo);

		rd.endSingleTimeCommands(commandBuffer);
	}

	// render target is no longer needed
	rt.destroy(_device);
	_device.destroySampler(sampler);
}

void EnvironmentEffects::filterSpecular(vk::ImageView srcCubemapView, uint32_t srcSize,
		uint32_t srcMipLevels, vk::Format format, vk::Image dstCubemap, uint32_t dstSize,
		uint32_t dstMipLevels) {
	assert(format == vk::Format::eR32G32B32A32Sfloat);

	vk::Sampler sampler = createSampler(_device, srcMipLevels);
	_updateFilterSet(srcCubemapView, sampler);

	uint32_t levelSize = dstSize;

	RD &rd = RD::getSingleton();

	for (uint32_t level = 0; level < dstMipLevels; level++) {
		float roughness = static_cast<float>(level) / static_cast<float>(dstMipLevels - 1);

		RenderTarget rt = RenderTarget::create(_device, levelSize, _memProperties);

		{
			vk::CommandBuffer commandBuffer = rd.beginSingleTimeCommands();
			_drawSpecularFilter(commandBuffer, rt, srcSize, roughness);
			rd.endSingleTimeCommands(commandBuffer);
		}

		vk::Image framebufferImage = rt.getColorAttachment().getImage();

		rd.imageLayoutTransition(framebufferImage, format, 1, 6,
				vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal);

		_copyImageToLevel(framebufferImage, dstCubemap, level, levelSize);

		// render target is no longer needed
		rt.destroy(_device);
		levelSize = levelSize >> 1;
	}

	_device.destroySampler(sampler);
}

void EnvironmentEffects::init() {
	RD &rd = RD::getSingleton();

	_device = rd.getDevice();
	_memProperties = rd.getPhysicalDevice().getMemoryProperties();

	vk::DescriptorPool descriptorPool = rd.getDescriptorPool();

	_createDescriptors(descriptorPool);
	_createPipelines();

	_initialized = true;
}

EnvironmentEffects::~EnvironmentEffects() {
	if (!_initialized)
		return;

	_device.destroyPipeline(_brdfPipeline);
	_device.destroyPipelineLayout(_brdfPipelineLayout);
	_device.destroyDescriptorSetLayout(_brdfSetLayout);

	_device.destroyPipeline(_cubemapPipeline);
	_device.destroyPipelineLayout(_cubemapPipelineLayout);
	_device.destroyDescriptorSetLayout(_cubemapSetLayout);

	_device.destroyPipeline(_irradiancePipeline);
	_device.destroyPipelineLayout(_irradiancePipelineLayout);

	_device.destroyPipeline(_specularPipeline);
	_device.destroyPipelineLayout(_specularPipelineLayout);

	_device.destroyDescriptorSetLayout(_filterSetLayout);
}
