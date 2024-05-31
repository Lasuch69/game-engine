#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "rendering_device.h"
#include "shaders/depth_material.gen.h"
#include "shaders/sky_effect.gen.h"
#include "shaders/standard_material.gen.h"
#include "shaders/tonemap_effect.gen.h"

#include "pipeline.h"
#include "types/vertex.h"

#include "material.h"

vk::PipelineLayout ShaderEffect::getPipelineLayout() const {
	return _pipelineLayout;
}

vk::Pipeline ShaderEffect::getPipeline() const {
	return _pipeline;
}

void ShaderEffect::destroy() {}

void DepthMaterial::create() {
	vk::PushConstantRange pushConstants;
	pushConstants.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	pushConstants.setSize(sizeof(MaterialPushConstants));

	_pipelineLayout = pipelineLayoutCreate(nullptr, 0, &pushConstants, 1);

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

	vk::RenderPass renderPass = RD::getSingleton().getRenderPass();
	vk::ResultValue<vk::Pipeline> pipeline =
			builder.pipelineCreate(vert, frag, _pipelineLayout, renderPass, 0);

	if (pipeline.result != vk::Result::eSuccess)
		throw std::runtime_error("DepthMaterial pipeline creation failed!");

	_pipeline = pipeline.value;

	shaderModuleDestroy(vert);
	shaderModuleDestroy(frag);
}

void StandardMaterial::create() {
	vk::PushConstantRange pushConstants;
	pushConstants.setStageFlags(vk::ShaderStageFlagBits::eVertex);
	pushConstants.setSize(sizeof(MaterialPushConstants));

	_pipelineLayout = pipelineLayoutCreate(nullptr, 0, &pushConstants, 1);

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

	vk::RenderPass renderPass = RD::getSingleton().getRenderPass();
	vk::ResultValue<vk::Pipeline> pipeline =
			builder.pipelineCreate(vert, frag, _pipelineLayout, renderPass, 1);

	if (pipeline.result != vk::Result::eSuccess)
		throw std::runtime_error("StandardMaterial pipeline creation failed!");

	_pipeline = pipeline.value;

	shaderModuleDestroy(vert);
	shaderModuleDestroy(frag);
}

void SkyEffect::create() {
	vk::PushConstantRange pushConstants;
	pushConstants.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	pushConstants.setSize(sizeof(SkyEffectPushConstants));

	_pipelineLayout = pipelineLayoutCreate(nullptr, 0, &pushConstants, 1);

	SkyEffectShader spirv;

	vk::ShaderModule vert = shaderModuleCreate(spirv.vertexCode, sizeof(spirv.vertexCode));
	vk::ShaderModule frag = shaderModuleCreate(spirv.fragmentCode, sizeof(spirv.fragmentCode));

	PipelineBuilder builder = {};
	builder.setDepthTest(true);

	vk::RenderPass renderPass = RD::getSingleton().getRenderPass();
	vk::ResultValue<vk::Pipeline> pipeline =
			builder.pipelineCreate(vert, frag, _pipelineLayout, renderPass, 1);

	if (pipeline.result != vk::Result::eSuccess)
		throw std::runtime_error("SkyEffect pipeline creation failed!");

	_pipeline = pipeline.value;

	shaderModuleDestroy(vert);
	shaderModuleDestroy(frag);
}

void TonemapEffect::create() {
	vk::PushConstantRange pushConstants;
	pushConstants.setStageFlags(vk::ShaderStageFlagBits::eFragment);
	pushConstants.setSize(sizeof(TonemapEffectPushConstants));

	_pipelineLayout = pipelineLayoutCreate(nullptr, 0, &pushConstants, 1);

	TonemapEffectShader spirv;

	vk::ShaderModule vert = shaderModuleCreate(spirv.vertexCode, sizeof(spirv.vertexCode));
	vk::ShaderModule frag = shaderModuleCreate(spirv.fragmentCode, sizeof(spirv.fragmentCode));

	PipelineBuilder builder = {};

	vk::RenderPass renderPass = RD::getSingleton().getRenderPass();
	vk::ResultValue<vk::Pipeline> pipeline =
			builder.pipelineCreate(vert, frag, _pipelineLayout, renderPass, 2);

	if (pipeline.result != vk::Result::eSuccess)
		throw std::runtime_error("TonemapEffect pipeline creation failed!");

	_pipeline = pipeline.value;

	shaderModuleDestroy(vert);
	shaderModuleDestroy(frag);
}
