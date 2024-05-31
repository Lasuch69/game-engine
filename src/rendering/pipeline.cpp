#include <cstdint>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_structs.hpp>

#include "rendering_device.h"

#include "pipeline.h"

vk::ShaderModule shaderModuleCreate(const uint32_t *pCode, size_t codeSize) {
	vk::ShaderModuleCreateInfo createInfo;
	createInfo.setPCode(pCode);
	createInfo.setCodeSize(codeSize);

	return RD::getSingleton().getDevice().createShaderModule(createInfo);
}

void shaderModuleDestroy(vk::ShaderModule shaderModule) {
	return RD::getSingleton().getDevice().destroyShaderModule(shaderModule);
}

vk::PipelineLayout pipelineLayoutCreate(const vk::DescriptorSetLayout *pSetLayouts,
		uint32_t setLayoutCount, const vk::PushConstantRange *pPushConstantRanges,
		uint32_t pushConstantRangeCount) {
	vk::PipelineLayoutCreateInfo createInfo = {};
	createInfo.setPSetLayouts(pSetLayouts);
	createInfo.setSetLayoutCount(setLayoutCount);
	createInfo.setPPushConstantRanges(pPushConstantRanges);
	createInfo.setPushConstantRangeCount(pushConstantRangeCount);

	return RD::getSingleton().getDevice().createPipelineLayout(createInfo);
}

void PipelineBuilder::setSamples(vk::SampleCountFlagBits samples) {
	_samples = samples;
}

void PipelineBuilder::setDepthTest(vk::Bool32 enable) {
	_depthTestEnable = enable;
}

void PipelineBuilder::setDepthWrite(vk::Bool32 enable) {
	_depthWriteEnable = enable;
}

void PipelineBuilder::setVertexInput(vk::PipelineVertexInputStateCreateInfo vertexInput) {
	_vertexInput = vertexInput;
}

vk::ResultValue<vk::Pipeline> PipelineBuilder::pipelineCreate(vk::ShaderModule vertexModule,
		vk::ShaderModule fragmentModule, vk::PipelineLayout pipelineLayout,
		vk::RenderPass renderPass, uint32_t subpass) {
	vk::PipelineShaderStageCreateInfo vertexStageInfo = {};
	vertexStageInfo.setModule(vertexModule);
	vertexStageInfo.setStage(vk::ShaderStageFlagBits::eVertex);
	vertexStageInfo.setPName("main");

	vk::PipelineShaderStageCreateInfo fragmentStageInfo = {};
	fragmentStageInfo.setModule(fragmentModule);
	fragmentStageInfo.setStage(vk::ShaderStageFlagBits::eFragment);
	fragmentStageInfo.setPName("main");

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertexStageInfo, fragmentStageInfo };

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);

	vk::PipelineViewportStateCreateInfo viewportState = {};
	viewportState.setViewportCount(1);
	viewportState.setScissorCount(1);

	vk::PipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.setDepthBiasEnable(VK_FALSE);
	rasterizer.setRasterizerDiscardEnable(VK_FALSE);
	rasterizer.setPolygonMode(vk::PolygonMode::eFill);
	rasterizer.setLineWidth(1.0f);
	rasterizer.setCullMode(vk::CullModeFlagBits::eBack);
	rasterizer.setFrontFace(vk::FrontFace::eCounterClockwise);
	rasterizer.setDepthBiasEnable(VK_FALSE);

	vk::PipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.setSampleShadingEnable(VK_FALSE);
	multisampling.setRasterizationSamples(_samples);

	vk::PipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.setDepthTestEnable(_depthTestEnable);
	depthStencil.setDepthWriteEnable(_depthWriteEnable);
	depthStencil.setDepthCompareOp(vk::CompareOp::eGreaterOrEqual);
	depthStencil.setDepthBoundsTestEnable(VK_FALSE);
	depthStencil.setStencilTestEnable(VK_FALSE);

	vk::ColorComponentFlags colorWriteMask =
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.setColorWriteMask(colorWriteMask);
	colorBlendAttachment.setBlendEnable(VK_FALSE);

	vk::PipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.setLogicOpEnable(VK_FALSE);
	colorBlending.setLogicOp(vk::LogicOp::eCopy);
	colorBlending.setAttachments(colorBlendAttachment);
	colorBlending.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

	std::vector<vk::DynamicState> dynamicStates = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};

	vk::PipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.setDynamicStates(dynamicStates);

	vk::GraphicsPipelineCreateInfo createInfo = {};
	createInfo.setStages(shaderStages);
	createInfo.setPVertexInputState(&_vertexInput);
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

	return RD::getSingleton().getDevice().createGraphicsPipeline(nullptr, createInfo);
}
