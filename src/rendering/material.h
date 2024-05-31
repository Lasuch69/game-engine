#ifndef MATERIAL_H
#define MATERIAL_H

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

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

class ShaderEffect {
protected:
	vk::PipelineLayout _pipelineLayout;
	vk::Pipeline _pipeline;

public:
	vk::PipelineLayout getPipelineLayout() const;
	vk::Pipeline getPipeline() const;

	virtual void create() = 0;
	void destroy();
};

class DepthMaterial : public ShaderEffect {
public:
	void create();
};

class StandardMaterial : public ShaderEffect {
public:
	void create();
};

class SkyEffect : public ShaderEffect {
	void create();
};

class TonemapEffect : public ShaderEffect {
public:
	void create();
};

#endif // !MATERIAL_H
