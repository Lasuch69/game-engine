#ifndef RENDERING_DEVICE_H
#define RENDERING_DEVICE_H

#include <cstdint>
#include <optional>

#include <glm/glm.hpp>

#include "vertex.h"
#include "vk_types.h"
#include "vulkan_context.h"

const int FRAMES_IN_FLIGHT = 2;
const int MAX_LIGHT_COUNT = 8;

struct UniformBufferObject {
	glm::mat4 projView;
	glm::mat4 view;
	uint32_t lightCount;
};

struct MeshPushConstants {
	glm::mat4 model;
	glm::mat4 modelViewNormal;
};

struct Mesh {
	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;
	uint32_t indexCount;
};

struct LightData {
	glm::vec3 position;
	float range;
	glm::vec3 color;
	float intensity;
};

class RenderingDevice {
private:
	VulkanContext *_pContext;
	uint32_t _frame = 0;

	uint32_t _width, _height;
	bool _resized;

	VmaAllocator _allocator;
	vk::CommandBuffer _commandBuffers[FRAMES_IN_FLIGHT];

	vk::Semaphore _presentSemaphores[FRAMES_IN_FLIGHT];
	vk::Semaphore _renderSemaphores[FRAMES_IN_FLIGHT];
	vk::Fence _fences[FRAMES_IN_FLIGHT];

	vk::DescriptorPool _descriptorPool;

	vk::DescriptorSetLayout _uniformLayout;
	vk::DescriptorSetLayout _inputAttachmentLayout;
	vk::DescriptorSetLayout _lightLayout;

	vk::DescriptorSet _uniformSets[FRAMES_IN_FLIGHT];
	vk::DescriptorSet _inputAttachmentSet;
	vk::DescriptorSet _lightSet;

	AllocatedBuffer _uniformBuffers[FRAMES_IN_FLIGHT];
	VmaAllocationInfo _uniformAllocInfos[FRAMES_IN_FLIGHT];
	AllocatedBuffer _lightBuffer;

	vk::PipelineLayout _materialLayout;
	vk::Pipeline _materialPipeline;

	vk::PipelineLayout _tonemapLayout;
	vk::Pipeline _tonemapPipeline;

	std::optional<uint32_t> _imageIndex;

	vk::CommandBuffer _beginSingleTimeCommands();
	void _endSingleTimeCommands(vk::CommandBuffer commandBuffer);

	AllocatedBuffer _bufferCreate(
			vk::BufferUsageFlags usage, vk::DeviceSize size, VmaAllocationInfo *pAllocInfo = NULL);

	void _bufferCopy(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

	void _bufferSend(vk::Buffer dstBuffer, uint8_t *data, size_t size);

public:
	void init(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void windowResize(uint32_t width, uint32_t height);

	Mesh meshCreate(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
	void meshDestroy(Mesh &mesh);

	void updateUniformBuffer(const glm::mat4 &proj, const glm::mat4 &view, uint32_t lightCount);
	void updateLightBuffer(LightData *lights, uint64_t lightCount);

	vk::Instance getInstance() const;
	vk::PipelineLayout getPipelineLayout() const;

	vk::CommandBuffer drawBegin();
	void drawEnd(vk::CommandBuffer commandBuffer);

	RenderingDevice(bool useValidation);
};

#endif // !RENDERING_DEVICE_H
