#ifndef RENDERING_DEVICE_H
#define RENDERING_DEVICE_H

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "camera.h"
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

struct LightData {
	glm::vec3 position;
	float range;
	glm::vec3 color;
	float intensity;
};

struct Mesh {
	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;
	uint32_t indexCount;

	glm::mat4 transform;
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

	std::vector<Mesh> _meshes;
	std::vector<LightData> _lights;
	Camera *_pCamera = new Camera;

	vk::CommandBuffer _beginSingleTimeCommands();
	void _endSingleTimeCommands(vk::CommandBuffer commandBuffer);

	void _copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
	void _sendToBuffer(vk::Buffer dstBuffer, uint8_t *data, size_t size);

	Mesh _uploadMesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

	void _updateUniformBuffer(const Camera *pCamera);

public:
	void createMesh(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices,
			const glm::mat4 &transform);

	void createLight(const glm::vec3 &position, const glm::vec3 &color, float intensity);

	void createCamera(const glm::mat4 transform, float fovY, float zNear, float zFar);

	void draw();

	void createWindow(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void resizeWindow(uint32_t width, uint32_t height);

	vk::Instance getInstance();

	RenderingDevice(std::vector<const char *> extensions, bool useValidation);
};

#endif // !RENDERING_DEVICE_H
