#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>

#include "shaders/depth_material.gen.h"
#include "shaders/standard_material.gen.h"

#include "shaders/sky_effect.gen.h"
#include "shaders/tonemap_effect.gen.h"

#include "pipeline.h"
#include "storage/light_storage.h"
#include "types/vertex.h"

#include "rendering_device.h"

#include "render_scene_forward.h"

void RenderSceneForward::_depthMaterialInit() {
	vk::PushConstantRange pushConstants;
	pushConstants.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	pushConstants.setSize(sizeof(MaterialPushConstants));

	_depthMaterialPipelineLayout = pipelineLayoutCreate(nullptr, 0, &pushConstants, 1);

	DepthMaterialShader spirv;

	vk::ShaderModule vert = shaderModuleCreate(spirv.vertexCode, sizeof(spirv.vertexCode));
	vk::ShaderModule frag = shaderModuleCreate(spirv.fragmentCode, sizeof(spirv.fragmentCode));

	auto binding = Vertex::getBindingDescription();
	auto attribute = Vertex::getAttributeDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInput;
	vertexInput.setVertexBindingDescriptions(binding);
	vertexInput.setVertexAttributeDescriptions(attribute);

	PipelineBuilder builder = {};
	builder.setDepthTest(true);
	builder.setDepthWrite(true);
	builder.setVertexInput(vertexInput);

	vk::ResultValue<vk::Pipeline> pipeline = builder.pipelineCreate(
			vert, frag, _depthMaterialPipelineLayout, _rt.getRenderPass(), 0);

	if (pipeline.result != vk::Result::eSuccess)
		throw std::runtime_error("DepthMaterial pipeline creation failed!");

	_depthMaterialPipeline = pipeline.value;

	shaderModuleDestroy(vert);
	shaderModuleDestroy(frag);
}

void RenderSceneForward::_standardMaterialInit() {
	vk::PushConstantRange pushConstants;
	pushConstants.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	pushConstants.setSize(sizeof(MaterialPushConstants));

	_standardMaterialPipelineLayout = pipelineLayoutCreate(nullptr, 0, &pushConstants, 1);

	StandardMaterialShader spirv;

	vk::ShaderModule vert = shaderModuleCreate(spirv.vertexCode, sizeof(spirv.vertexCode));
	vk::ShaderModule frag = shaderModuleCreate(spirv.fragmentCode, sizeof(spirv.fragmentCode));

	auto binding = Vertex::getBindingDescription();
	auto attribute = Vertex::getAttributeDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInput;
	vertexInput.setVertexBindingDescriptions(binding);
	vertexInput.setVertexAttributeDescriptions(attribute);

	PipelineBuilder builder = {};
	builder.setDepthTest(true);
	builder.setVertexInput(vertexInput);

	vk::ResultValue<vk::Pipeline> pipeline = builder.pipelineCreate(
			vert, frag, _standardMaterialPipelineLayout, _rt.getRenderPass(), 1);

	if (pipeline.result != vk::Result::eSuccess)
		throw std::runtime_error("StandardMaterial pipeline creation failed!");

	_standardMaterialPipeline = pipeline.value;

	shaderModuleDestroy(vert);
	shaderModuleDestroy(frag);
}

void RenderSceneForward::_skyEffectInit() {
	vk::PushConstantRange pushConstants;
	pushConstants.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	pushConstants.setSize(sizeof(SkyEffectPushConstants));

	_skyEffectPipelineLayout = pipelineLayoutCreate(nullptr, 0, &pushConstants, 1);

	SkyEffectShader spirv;

	vk::ShaderModule vert = shaderModuleCreate(spirv.vertexCode, sizeof(spirv.vertexCode));
	vk::ShaderModule frag = shaderModuleCreate(spirv.fragmentCode, sizeof(spirv.fragmentCode));

	PipelineBuilder builder = {};
	builder.setDepthTest(true);

	vk::ResultValue<vk::Pipeline> pipeline =
			builder.pipelineCreate(vert, frag, _skyEffectPipelineLayout, _rt.getRenderPass(), 1);

	if (pipeline.result != vk::Result::eSuccess)
		throw std::runtime_error("SkyEffect pipeline creation failed!");

	_skyEffectPipeline = pipeline.value;

	shaderModuleDestroy(vert);
	shaderModuleDestroy(frag);
}

void RenderSceneForward::_tonemapEffectInit() {
	vk::PushConstantRange pushConstants;
	pushConstants.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	pushConstants.setSize(sizeof(TonemapEffectPushConstants));

	_tonemapEffectPipelineLayout =
			pipelineLayoutCreate(&_tonemapInputSetLayout, 1, &pushConstants, 1);

	TonemapEffectShader spirv;

	vk::ShaderModule vert = shaderModuleCreate(spirv.vertexCode, sizeof(spirv.vertexCode));
	vk::ShaderModule frag = shaderModuleCreate(spirv.fragmentCode, sizeof(spirv.fragmentCode));

	PipelineBuilder builder = {};

	vk::RenderPass renderPass = RD::getSingleton().getRenderPass();
	vk::ResultValue<vk::Pipeline> pipeline =
			builder.pipelineCreate(vert, frag, _tonemapEffectPipelineLayout, renderPass, 0);

	if (pipeline.result != vk::Result::eSuccess)
		throw std::runtime_error("TonemapEffect pipeline creation failed!");

	_tonemapEffectPipeline = pipeline.value;

	shaderModuleDestroy(vert);
	shaderModuleDestroy(frag);
}

void RenderSceneForward::_lightUpdate() {
	LS &ls = LS::getSingleton();

	{
		void *pDst = _directionalLightAllocInfo.pMappedData;
		uint8_t *pData = ls.getDirectionalLightBuffer();
		size_t size = ls.getDirectionalLightSSBOSize();
		memcpy(pDst, pData, size);
	}

	{
		void *pDst = _pointLightAllocInfo.pMappedData;
		uint8_t *pData = ls.getPointLightBuffer();
		size_t size = ls.getPointLightSSBOSize();
		memcpy(pDst, pData, size);
	}
}

void RenderSceneForward::_drawScene(vk::CommandBuffer commandBuffer) {
	vk::Extent2D extent = _rt.getExtent();

	vk::Viewport viewport;
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setWidth(extent.width);
	viewport.setHeight(extent.height);
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Rect2D scissor;
	scissor.setOffset({ 0, 0 });
	scissor.setExtent(extent);

	vk::Rect2D renderArea;
	renderArea.setOffset({ 0, 0 });
	renderArea.setExtent(extent);

	std::array<vk::ClearValue, 2> clearValues;
	clearValues[0].depthStencil = vk::ClearDepthStencilValue(0.0f, 0);
	clearValues[1].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.setRenderPass(_rt.getRenderPass());
	renderPassInfo.setFramebuffer(_rt.getFramebuffer());
	renderPassInfo.setRenderArea(renderArea);
	renderPassInfo.setClearValues(clearValues);

	commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

	commandBuffer.setViewport(0, viewport);
	commandBuffer.setScissor(0, scissor);

	commandBuffer.nextSubpass(vk::SubpassContents::eInline);

	commandBuffer.endRenderPass();
}

void RenderSceneForward::_drawTonemap(vk::CommandBuffer commandBuffer) {
	vk::ClearValue clearValue;
	clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

	RD &rd = RD::getSingleton();
	vk::Extent2D extent = rd.getSwapchainExtent();

	vk::Viewport viewport;
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setWidth(extent.width);
	viewport.setHeight(extent.height);
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Rect2D scissor;
	scissor.setOffset({ 0, 0 });
	scissor.setExtent(extent);

	vk::Rect2D renderArea;
	renderArea.setOffset({ 0, 0 });
	renderArea.setExtent(extent);

	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.setRenderPass(rd.getRenderPass());
	renderPassInfo.setFramebuffer(rd.getFramebuffer());
	renderPassInfo.setRenderArea(renderArea);
	renderPassInfo.setClearValues(clearValue);

	commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

	commandBuffer.setViewport(0, viewport);
	commandBuffer.setScissor(0, scissor);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _tonemapEffectPipeline);
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _tonemapEffectPipelineLayout,
			0, _tonemapInputSet, nullptr);

	TonemapEffectPushConstants constants = {};
	constants.exposure = 1.25;
	constants.white = 8.0;

	commandBuffer.pushConstants(_tonemapEffectPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0,
			sizeof(constants), &constants);

	commandBuffer.draw(3, 1, 0, 0);

	commandBuffer.endRenderPass();
}

void RenderSceneForward::uniformBufferUpdate(const glm::vec3 &viewPosition) {
	UniformBufferObject ubo = {};
	ubo.directionalLightCount = LS::getSingleton().getDirectionalLightCount();
	ubo.pointLightCount = LS::getSingleton().getPointLightCount();
	memcpy(ubo.viewPosition, &viewPosition, sizeof(float) * 3);

	uint32_t idx = RD::getSingleton().getFrameIndex();

	memcpy(_uniformAllocInfos[idx].pMappedData, &ubo, sizeof(ubo));
}

void RenderSceneForward::draw() {
	_lightUpdate();

	RD &rd = RD::getSingleton();
	vk::CommandBuffer commandBuffer = rd.drawBegin();

	_drawScene(commandBuffer);

	rd.imageLayoutTransition(_rt.getColor().getImage(), vk::Format::eB10G11R11UfloatPack32, 1, 1,
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	_drawTonemap(commandBuffer);

	rd.drawEnd(commandBuffer);
}

void RenderSceneForward::init() {
	RD &rd = RD::getSingleton();
	LS &ls = LS::getSingleton();

	vk::Device device = rd.getDevice();

	{
		// uniform

		vk::DescriptorSetLayoutBinding binding;
		binding.setBinding(0);
		binding.setDescriptorType(vk::DescriptorType::eUniformBuffer);
		binding.setDescriptorCount(1);
		binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::DescriptorSetLayoutCreateInfo createInfo;
		createInfo.setBindings(binding);

		vk::Result err = device.createDescriptorSetLayout(&createInfo, nullptr, &_uniformSetLayout);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Uniform set layout creation failed!");

		std::vector<vk::DescriptorSetLayout> layouts(FRAMES_IN_FLIGHT, _uniformSetLayout);

		vk::DescriptorSetAllocateInfo allocInfo;
		allocInfo.setDescriptorPool(rd.getDescriptorPool());
		allocInfo.setDescriptorSetCount(FRAMES_IN_FLIGHT);
		allocInfo.setSetLayouts(layouts);

		std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT> uniformSets{};

		err = device.allocateDescriptorSets(&allocInfo, uniformSets.data());

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Uniform set allocation failed!");

		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			_uniformBuffers[i] = rd.bufferCreate(
					vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
					sizeof(UniformBufferObject), &_uniformAllocInfos[i]);

			_uniformSets[i] = uniformSets[i];

			vk::DescriptorBufferInfo bufferInfo = _uniformBuffers[i].getBufferInfo();

			vk::WriteDescriptorSet writeInfo;
			writeInfo.setDstSet(_uniformSets[i]);
			writeInfo.setDstBinding(0);
			writeInfo.setDstArrayElement(0);
			writeInfo.setDescriptorType(vk::DescriptorType::eUniformBuffer);
			writeInfo.setDescriptorCount(1);
			writeInfo.setBufferInfo(bufferInfo);

			device.updateDescriptorSets(writeInfo, nullptr);
		}
	}

	{
		// lights

		std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {};
		bindings[0].setBinding(0);
		bindings[0].setDescriptorType(vk::DescriptorType::eStorageBuffer);
		bindings[0].setDescriptorCount(1);
		bindings[0].setStageFlags(vk::ShaderStageFlagBits::eFragment);

		bindings[1].setBinding(1);
		bindings[1].setDescriptorType(vk::DescriptorType::eStorageBuffer);
		bindings[1].setDescriptorCount(1);
		bindings[1].setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.setBindings(bindings);

		vk::Result err = device.createDescriptorSetLayout(&createInfo, nullptr, &_lightSetLayout);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Light descriptor set layout creation failed!");

		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.setDescriptorPool(rd.getDescriptorPool());
		allocInfo.setDescriptorSetCount(1);
		allocInfo.setSetLayouts(_lightSetLayout);

		err = device.allocateDescriptorSets(&allocInfo, &_lightSet);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Light descriptor set allocation failed!");

		_directionalLightSSBO = rd.bufferCreate(
				vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
				ls.getDirectionalLightSSBOSize(), &_directionalLightAllocInfo);

		_pointLightSSBO = rd.bufferCreate(
				vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
				ls.getPointLightSSBOSize(), &_pointLightAllocInfo);
	}

	{
		// tonemap input

		vk::DescriptorSetLayoutBinding binding = {};
		binding.setBinding(0);
		binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
		binding.setDescriptorCount(1);
		binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.setBindings(binding);

		vk::Result err =
				device.createDescriptorSetLayout(&createInfo, nullptr, &_tonemapInputSetLayout);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Tonemap input set layout creation failed!");

		vk::DescriptorSetAllocateInfo allocInfo = {};
		allocInfo.setDescriptorPool(rd.getDescriptorPool());
		allocInfo.setDescriptorSetCount(1);
		allocInfo.setSetLayouts(_tonemapInputSetLayout);

		err = device.allocateDescriptorSets(&allocInfo, &_tonemapInputSet);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Tonemap input set allocation failed!");
	}

	_rt.create(800, 600, vk::Format::eB10G11R11UfloatPack32);

	vk::Sampler sampler = rd.samplerCreate(vk::Filter::eNearest, 1);

	vk::DescriptorImageInfo imageInfo;
	imageInfo.setImageView(_rt.getColor().getImageView());
	imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfo.setSampler(sampler);

	vk::WriteDescriptorSet writeInfo;
	writeInfo.setDstSet(_tonemapInputSet);
	writeInfo.setDstBinding(0);
	writeInfo.setDstArrayElement(0);
	writeInfo.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeInfo.setDescriptorCount(1);
	writeInfo.setImageInfo(imageInfo);

	device.updateDescriptorSets(writeInfo, nullptr);

	_depthMaterialInit();
	//_standardMaterialInit();

	//_skyEffectInit();
	_tonemapEffectInit();
}
