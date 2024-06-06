#ifndef MATERIAL_H
#define MATERIAL_H

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include <glm/glm.hpp>

typedef struct {
	float viewPosition[3];
	unsigned int directionalLightCount;
	unsigned int pointLightCount;
} UniformBufferObject;

typedef struct {
	float projView[16];
	float model[16];
} MeshConstants;

typedef struct {
	vk::PipelineLayout layout;
	vk::Pipeline handle;
} Pipeline;

enum class MaterialPass {
	DepthOnly = 0,
	Color = 1,
	Tonemap = 2,
};

class DepthMaterial {
private:
	Pipeline _pipeline;

public:
	void create(vk::Device device, vk::RenderPass renderPass);

	inline vk::PipelineLayout getPipelineLayout() const {
		return _pipeline.layout;
	}

	inline vk::Pipeline getPipeline() const {
		return _pipeline.handle;
	}
};

class StandardMaterialInstance;

class StandardMaterial {
private:
	vk::DescriptorSetLayout _environmentSetLayout;
	vk::DescriptorSetLayout _textureSetLayout;

	Pipeline _pipeline;

public:
	void create(vk::Device device, vk::RenderPass renderPass);
	StandardMaterialInstance createInstance();
};

class StandardMaterialInstance {
	vk::DescriptorSet _environmentSet;
	vk::DescriptorSet _textureSet;

	Pipeline _pipeline;

private:
	inline vk::PipelineLayout getPipelineLayout() const {
		return _pipeline.layout;
	}

	inline vk::Pipeline getPipeline() const {
		return _pipeline.handle;
	}

	StandardMaterialInstance(Pipeline pipeline);
};

class SkyEffect {
private:
	vk::DescriptorSetLayout _skyTextureSetLayout;
	vk::DescriptorSet _skyTextureSet;

	Pipeline _pipeline;

	typedef struct {
		float invProj[16];
		float invView[16];
	} SkyConstants;

public:
	void create(vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass);

	void updateTexture(vk::Device device, vk::ImageView imageView, vk::Sampler sampler);
	void draw(vk::CommandBuffer commandBuffer, const glm::mat4 &invProj, const glm::mat4 &invView);
};

class TonemapEffect {
private:
	vk::DescriptorSetLayout _tonemapInputSetLayout;
	vk::DescriptorSet _tonemapInputSet;

	Pipeline _pipeline;

	typedef struct {
		float exposure;
		float white;
	} TonemapConstants;

public:
	void create(vk::Device device, vk::DescriptorPool descriptorPool, vk::RenderPass renderPass);

	void updateInput(vk::Device device, vk::ImageView imageView);
	void draw(vk::CommandBuffer commandBuffer, float exposure, float white);
};

#endif // !MATERIAL_H
