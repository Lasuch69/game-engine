#include <cstdint>
#include <stdexcept>

#include <rendering/rendering_device.h>
#include <rendering/types/attachment.h>

#include "shaders/brdf.gen.h"
#include "shaders/irradiance_filter.gen.h"
#include "shaders/specular_filter.gen.h"

#include "effects.h"

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

void BRDFEffect::create(
		vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass) {
	vk::PipelineLayoutCreateInfo createInfo;
	_pipeline.layout = device.createPipelineLayout(createInfo);

	BrdfShader code;

	vk::ShaderModule vertex = createModule(device, code.vertexCode, sizeof(code.vertexCode));
	vk::ShaderModule fragment = createModule(device, code.fragmentCode, sizeof(code.fragmentCode));

	_pipeline.handle = createPipeline(device, vertex, fragment, _pipeline.layout, renderPass);

	device.destroyShaderModule(vertex);
	device.destroyShaderModule(fragment);
}

void BRDFEffect::draw(vk::CommandBuffer commandBuffer) {
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline.handle);
	commandBuffer.draw(3, 1, 0, 0);
}

void IrradianceFilterEffect::create(
		vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass) {
	vk::DescriptorSetLayoutBinding binding = {};
	binding.setBinding(0);
	binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	binding.setDescriptorCount(1);
	binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.setBindings(binding);

	vk::Result err =
			device.createDescriptorSetLayout(&layoutCreateInfo, nullptr, &_sourceSetLayout);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Failed to create source set layout!");

	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.setDescriptorPool(descriptorPool);
	allocInfo.setDescriptorSetCount(1);
	allocInfo.setSetLayouts(_sourceSetLayout);

	err = device.allocateDescriptorSets(&allocInfo, &_sourceSet);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Failed to allocate source set!");

	vk::PipelineLayoutCreateInfo createInfo;
	createInfo.setSetLayouts(_sourceSetLayout);

	_pipeline.layout = device.createPipelineLayout(createInfo);

	IrradianceFilterShader code;

	vk::ShaderModule vertex = createModule(device, code.vertexCode, sizeof(code.vertexCode));
	vk::ShaderModule fragment = createModule(device, code.fragmentCode, sizeof(code.fragmentCode));

	_pipeline.handle = createPipeline(device, vertex, fragment, _pipeline.layout, renderPass);

	device.destroyShaderModule(vertex);
	device.destroyShaderModule(fragment);
}

void IrradianceFilterEffect::updateSource(
		vk::Device device, vk::ImageView imageView, vk::Sampler sampler) {
	vk::DescriptorImageInfo imageInfo = {};
	imageInfo.setImageView(imageView);
	imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfo.setSampler(sampler);

	vk::WriteDescriptorSet writeInfo = {};
	writeInfo.setDstSet(_sourceSet);
	writeInfo.setDstBinding(0);
	writeInfo.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeInfo.setDescriptorCount(1);
	writeInfo.setImageInfo(imageInfo);

	device.updateDescriptorSets(writeInfo, nullptr);
}

void IrradianceFilterEffect::draw(vk::CommandBuffer commandBuffer) {
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline.handle);
	commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, _pipeline.layout, 0, _sourceSet, nullptr);

	commandBuffer.draw(3, 1, 0, 0);
}

void SpecularFilterEffect::create(
		vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass) {
	vk::DescriptorSetLayoutBinding binding = {};
	binding.setBinding(0);
	binding.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	binding.setDescriptorCount(1);
	binding.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	vk::DescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.setBindings(binding);

	vk::Result err =
			device.createDescriptorSetLayout(&layoutCreateInfo, nullptr, &_sourceSetLayout);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Failed to create source set layout!");

	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.setDescriptorPool(descriptorPool);
	allocInfo.setDescriptorSetCount(1);
	allocInfo.setSetLayouts(_sourceSetLayout);

	err = device.allocateDescriptorSets(&allocInfo, &_sourceSet);

	if (err != vk::Result::eSuccess)
		throw std::runtime_error("Failed to allocate source set!");

	vk::PushConstantRange pushConstantRange;
	pushConstantRange.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	pushConstantRange.setSize(sizeof(SpecularFilterConstants));

	vk::PipelineLayoutCreateInfo createInfo;
	createInfo.setPushConstantRanges(pushConstantRange);
	createInfo.setSetLayouts(_sourceSetLayout);

	_pipeline.layout = device.createPipelineLayout(createInfo);

	SpecularFilterShader code;

	vk::ShaderModule vertex = createModule(device, code.vertexCode, sizeof(code.vertexCode));
	vk::ShaderModule fragment = createModule(device, code.fragmentCode, sizeof(code.fragmentCode));

	_pipeline.handle = createPipeline(device, vertex, fragment, _pipeline.layout, renderPass);

	device.destroyShaderModule(vertex);
	device.destroyShaderModule(fragment);
}

void SpecularFilterEffect::updateSource(
		vk::Device device, vk::ImageView imageView, vk::Sampler sampler) {
	vk::DescriptorImageInfo imageInfo = {};
	imageInfo.setImageView(imageView);
	imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfo.setSampler(sampler);

	vk::WriteDescriptorSet writeInfo = {};
	writeInfo.setDstSet(_sourceSet);
	writeInfo.setDstBinding(0);
	writeInfo.setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeInfo.setDescriptorCount(1);
	writeInfo.setImageInfo(imageInfo);

	device.updateDescriptorSets(writeInfo, nullptr);
}

void SpecularFilterEffect::draw(
		vk::CommandBuffer commandBuffer, uint32_t size, uint32_t roughness) {
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline.handle);
	commandBuffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, _pipeline.layout, 0, _sourceSet, nullptr);

	SpecularFilterConstants constants = {};
	constants.size = size;
	constants.roughness = roughness;

	commandBuffer.pushConstants(
			_pipeline.layout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(constants), &constants);

	commandBuffer.draw(3, 1, 0, 0);
}
