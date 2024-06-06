#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "rendering/shaders/sky.gen.h"
#include "rendering/shaders/tonemap.gen.h"

#include "material.h"

static vk::ShaderModule createModule(vk::Device device, const uint32_t *pCode, size_t codeSize) {
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.setPCode(pCode);
	createInfo.setCodeSize(codeSize);

	return device.createShaderModule(createInfo);
}

static vk::Pipeline createPipeline(vk::Device device, vk::ShaderModule vertexModule,
		vk::ShaderModule fragmentModule, vk::PipelineLayout pipelineLayout,
		vk::RenderPass renderPass, uint32_t subpass,
		vk::PipelineVertexInputStateCreateInfo vertexInput = {}, bool writeDepth = false) {
	vk::PipelineShaderStageCreateInfo vertexStage;
	vertexStage.setModule(vertexModule);
	vertexStage.setStage(vk::ShaderStageFlagBits::eVertex);
	vertexStage.setPName("main");

	vk::PipelineShaderStageCreateInfo fragmentStage;
	fragmentStage.setModule(fragmentModule);
	fragmentStage.setStage(vk::ShaderStageFlagBits::eFragment);
	fragmentStage.setPName("main");

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertexStage, fragmentStage };

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
	depthStencil.setDepthTestEnable(VK_TRUE);
	depthStencil.setDepthWriteEnable(writeDepth);
	depthStencil.setDepthCompareOp(vk::CompareOp::eGreaterOrEqual);
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
	createInfo.setSubpass(subpass);

	vk::ResultValue<vk::Pipeline> result = device.createGraphicsPipeline(nullptr, createInfo);

	if (result.result != vk::Result::eSuccess)
		throw std::runtime_error("Graphics pipeline creation failed!");

	return result.value;
}

void SkyEffect::create(
		vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass) {
	vk::DescriptorSetLayoutBinding binding;
	binding.setBinding(0);
	binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	binding.setDescriptorCount(1);
	binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	descriptorSetLayoutCreateInfo.setBindings(binding);

	vk::Result err = device.createDescriptorSetLayout(
			&descriptorSetLayoutCreateInfo, nullptr, &_skyTextureSetLayout);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Sky texture set layout creation failed!");

	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
	descriptorSetAllocateInfo.setDescriptorPool(descriptorPool);
	descriptorSetAllocateInfo.setDescriptorSetCount(1);
	descriptorSetAllocateInfo.setSetLayouts(_skyTextureSetLayout);

	err = device.allocateDescriptorSets(&descriptorSetAllocateInfo, &_skyTextureSet);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Sky texture set allocation failed!");

	vk::PushConstantRange pushConstantRange;
	pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	pushConstantRange.setSize(sizeof(SkyConstants));

	vk::PipelineLayoutCreateInfo createInfo;
	createInfo.setSetLayouts(_skyTextureSetLayout);
	createInfo.setPushConstantRanges(pushConstantRange);

	_pipeline.layout = device.createPipelineLayout(createInfo);

	SkyShader spirv;

	vk::ShaderModule vertexModule =
			createModule(device, spirv.vertexCode, sizeof(spirv.vertexCode));

	vk::ShaderModule fragmentModule =
			createModule(device, spirv.fragmentCode, sizeof(spirv.fragmentCode));

	_pipeline.handle = createPipeline(device, vertexModule, fragmentModule, _pipeline.layout,
			renderPass, static_cast<uint32_t>(MaterialPass::Color));

	device.destroyShaderModule(vertexModule);
	device.destroyShaderModule(fragmentModule);
}

void SkyEffect::updateTexture(vk::Device device, vk::ImageView imageView, vk::Sampler sampler) {
	vk::DescriptorImageInfo imageInfo;
	imageInfo.setImageView(imageView);
	imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfo.setSampler(sampler);

	vk::WriteDescriptorSet writeInfo;
	writeInfo.setDstSet(_skyTextureSet);
	writeInfo.setDstBinding(0);
	writeInfo.setDstArrayElement(0);
	writeInfo.setDescriptorType(vk::DescriptorType::eInputAttachment);
	writeInfo.setDescriptorCount(1);
	writeInfo.setImageInfo(imageInfo);

	device.updateDescriptorSets(writeInfo, nullptr);
}

void SkyEffect::draw(
		vk::CommandBuffer commandBuffer, const glm::mat4 &invProj, const glm::mat4 &invView) {
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline.handle);
	commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, _pipeline.layout, 0, _skyTextureSet, nullptr);

	SkyConstants constants;
	memcpy(&constants.invProj, &invProj, sizeof(invProj));
	memcpy(&constants.invView, &invView, sizeof(invView));

	commandBuffer.pushConstants(
			_pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(constants), &constants);

	commandBuffer.draw(3, 1, 0, 0);
}

void TonemapEffect::create(
		vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass) {
	vk::DescriptorSetLayoutBinding binding;
	binding.setBinding(0);
	binding.setDescriptorType(vk::DescriptorType::eInputAttachment);
	binding.setDescriptorCount(1);
	binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	descriptorSetLayoutCreateInfo.setBindings(binding);

	vk::Result err = device.createDescriptorSetLayout(
			&descriptorSetLayoutCreateInfo, nullptr, &_tonemapInputSetLayout);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Tonemap input set layout creation failed!");

	vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
	descriptorSetAllocateInfo.setDescriptorPool(descriptorPool);
	descriptorSetAllocateInfo.setDescriptorSetCount(1);
	descriptorSetAllocateInfo.setSetLayouts(_tonemapInputSetLayout);

	err = device.allocateDescriptorSets(&descriptorSetAllocateInfo, &_tonemapInputSet);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Tonemap input set allocation failed!");

	vk::PushConstantRange pushConstantRange;
	pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	pushConstantRange.setSize(sizeof(TonemapConstants));

	vk::PipelineLayoutCreateInfo createInfo;
	createInfo.setSetLayouts(_tonemapInputSetLayout);
	createInfo.setPushConstantRanges(pushConstantRange);

	_pipeline.layout = device.createPipelineLayout(createInfo);

	TonemapShader spirv;

	vk::ShaderModule vertexModule =
			createModule(device, spirv.vertexCode, sizeof(spirv.vertexCode));

	vk::ShaderModule fragmentModule =
			createModule(device, spirv.fragmentCode, sizeof(spirv.fragmentCode));

	_pipeline.handle = createPipeline(device, vertexModule, fragmentModule, _pipeline.layout,
			renderPass, static_cast<uint32_t>(MaterialPass::Tonemap));

	device.destroyShaderModule(vertexModule);
	device.destroyShaderModule(fragmentModule);
}

void TonemapEffect::updateInput(vk::Device device, vk::ImageView imageView) {
	vk::DescriptorImageInfo imageInfo;
	imageInfo.setImageView(imageView);
	imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfo.setSampler(VK_NULL_HANDLE);

	vk::WriteDescriptorSet writeInfo;
	writeInfo.setDstSet(_tonemapInputSet);
	writeInfo.setDstBinding(0);
	writeInfo.setDstArrayElement(0);
	writeInfo.setDescriptorType(vk::DescriptorType::eInputAttachment);
	writeInfo.setDescriptorCount(1);
	writeInfo.setImageInfo(imageInfo);

	device.updateDescriptorSets(writeInfo, nullptr);
}

void TonemapEffect::draw(vk::CommandBuffer commandBuffer, float exposure, float white) {
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline.handle);
	commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, _pipeline.layout, 0, _tonemapInputSet, nullptr);

	TonemapConstants constants;
	constants.exposure = exposure;
	constants.white = white;

	commandBuffer.pushConstants(
			_pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(constants), &constants);

	commandBuffer.draw(3, 1, 0, 0);
}
