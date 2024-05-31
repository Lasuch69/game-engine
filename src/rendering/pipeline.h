#ifndef PIPELINE_H
#define PIPELINE_H

#include <cstdint>
#include <vulkan/vulkan.hpp>

vk::ShaderModule shaderModuleCreate(const uint32_t *pCode, size_t codeSize);
void shaderModuleDestroy(vk::ShaderModule shaderModule);

vk::PipelineLayout pipelineLayoutCreate(const vk::DescriptorSetLayout *pSetLayouts,
		uint32_t setLayoutCount, const vk::PushConstantRange *pPushConstantRanges,
		uint32_t pushConstantRangeCount);

class PipelineBuilder {
	vk::SampleCountFlagBits _samples = vk::SampleCountFlagBits::e1;
	vk::Bool32 _depthTestEnable = VK_FALSE;
	vk::Bool32 _depthWriteEnable = VK_FALSE;

	vk::PipelineVertexInputStateCreateInfo _vertexInput = {};

public:
	void setSamples(vk::SampleCountFlagBits samples);
	void setDepthTest(vk::Bool32 enable);
	void setDepthWrite(vk::Bool32 enable);
	void setVertexInput(vk::PipelineVertexInputStateCreateInfo vertexInput);

	vk::ResultValue<vk::Pipeline> pipelineCreate(vk::ShaderModule vertexModule,
			vk::ShaderModule fragmentModule, vk::PipelineLayout pipelineLayout,
			vk::RenderPass renderPass, uint32_t subpass);
};

#endif // !PIPELINE_H
