#ifndef RENDER_SCENE_FORWARD_H
#define RENDER_SCENE_FORWARD_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include "render_target.h"
#include "rendering_device.h"
#include "types/allocated.h"

class RenderSceneForward {
public:
	static RenderSceneForward &getSingleton() {
		static RenderSceneForward instance;
		return instance;
	}

private:
	RenderSceneForward() {}

	typedef struct {
		float viewPosition[3];
		uint32_t directionalLightCount;
		uint32_t pointLightCount;
	} UniformBufferObject;

	typedef struct {
		float projView[16];
		float model[16];
	} MaterialPushConstants;

	typedef struct {
		float invProj[16];
		float invView[16];
	} SkyEffectPushConstants;

	typedef struct {
		float exposure;
		float white;
	} TonemapEffectPushConstants;

	AllocatedBuffer _uniformBuffers[FRAMES_IN_FLIGHT];
	VmaAllocationInfo _uniformAllocInfos[FRAMES_IN_FLIGHT];

	vk::DescriptorSetLayout _uniformSetLayout;
	vk::DescriptorSet _uniformSets[FRAMES_IN_FLIGHT];

	AllocatedBuffer _directionalLightSSBO;
	VmaAllocationInfo _directionalLightAllocInfo;

	AllocatedBuffer _pointLightSSBO;
	VmaAllocationInfo _pointLightAllocInfo;

	vk::DescriptorSetLayout _lightSetLayout;
	vk::DescriptorSet _lightSet;

	vk::DescriptorSetLayout _tonemapInputSetLayout;
	vk::DescriptorSet _tonemapInputSet;

	vk::PipelineLayout _depthMaterialPipelineLayout;
	vk::Pipeline _depthMaterialPipeline;

	vk::PipelineLayout _standardMaterialPipelineLayout;
	vk::Pipeline _standardMaterialPipeline;

	vk::PipelineLayout _skyEffectPipelineLayout;
	vk::Pipeline _skyEffectPipeline;

	vk::PipelineLayout _tonemapEffectPipelineLayout;
	vk::Pipeline _tonemapEffectPipeline;

	RenderTarget _rt;

	void _depthMaterialInit();
	void _standardMaterialInit();

	void _skyEffectInit();
	void _tonemapEffectInit();

	void _lightUpdate();

	void _drawScene(vk::CommandBuffer commandBuffer);
	void _drawTonemap(vk::CommandBuffer commandBuffer);

public:
	RenderSceneForward(RenderSceneForward const &) = delete;
	void operator=(RenderSceneForward const &) = delete;

	void uniformBufferUpdate(const glm::vec3 &viewPosition);

	void draw();

	void init();
};

#endif // !RENDER_SCENE_FORWARD_H
