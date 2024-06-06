#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan.hpp>

typedef struct {
	vk::PipelineLayout layout;
	vk::Pipeline handle;
} Pipeline;

#endif // !PIPELINE_H
