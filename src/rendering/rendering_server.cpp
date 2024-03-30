#include <cstdint>
#include <iostream>

#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>

#include "rendering_device.h"
#include "rendering_server.h"

#define CHECK_IF_VALID(owner, id, what)                                                            \
	if (!owner.has(id)) {                                                                          \
		std::cout << "ERROR: " << what << ": " << id << " is not valid resource!" << std::endl;    \
		return Error::InvalidResource;                                                             \
	}

const glm::vec3 CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 CAMERA_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);

glm::mat4 RenderingServer::CameraRS::viewMatrix() const {
	glm::vec3 position = glm::vec3(transform[3]);
	glm::mat3 rotation = glm::mat3(transform);

	glm::vec3 front = glm::normalize(rotation * CAMERA_FRONT);
	return glm::lookAtRH(position, position + front, CAMERA_UP);
}

glm::mat4 RenderingServer::CameraRS::projectionMatrix(float aspect) const {
	glm::mat4 projection = glm::perspective(fovY, aspect, zNear, zFar);
	projection[1][1] *= -1;

	return projection;
}

LightData RenderingServer::LightRS::toRaw() const {
	LightData lightData{};
	lightData.position = position;
	lightData.range = range;
	lightData.color = color;
	lightData.intensity = intensity;

	return lightData;
}

void RenderingServer::_updateLights() {
	if (_lights.size() == 0) {
		LightData lightData{};
		_pDevice->updateLightBuffer((uint8_t *)&lightData, sizeof(LightData));
		return;
	}

	std::vector<LightData> lights;
	for (const auto &[id, light] : _lights.map()) {
		if (lights.size() >= 8)
			break;

		lights.push_back(light.toRaw());
	}

	_pDevice->updateLightBuffer((uint8_t *)lights.data(), sizeof(LightData) * lights.size());
}

Error RenderingServer::cameraSetTransform(const glm::mat4 &transform) {
	_camera.transform = transform;

	return Error::None;
}

Error RenderingServer::cameraSetFovY(float fovY) {
	_camera.fovY = fovY;

	return Error::None;
}

Error RenderingServer::cameraSetZNear(float zNear) {
	_camera.zNear = zNear;

	return Error::None;
}

Error RenderingServer::cameraSetZFar(float zFar) {
	_camera.zFar = zFar;

	return Error::None;
}

Mesh RenderingServer::meshCreate(
		const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
	vk::DeviceSize vertexSize = sizeof(Vertex) * vertices.size();
	AllocatedBuffer vertexBuffer = _pDevice->bufferCreate(
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vertexSize);
	_pDevice->bufferSend(vertexBuffer.buffer, (uint8_t *)vertices.data(), (size_t)vertexSize);

	vk::DeviceSize indexSize = sizeof(uint32_t) * indices.size();
	AllocatedBuffer indexBuffer = _pDevice->bufferCreate(
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			indexSize);
	_pDevice->bufferSend(indexBuffer.buffer, (uint8_t *)indices.data(), (size_t)indexSize);

	MeshRS data = MeshRS{ vertexBuffer, indexBuffer, static_cast<uint32_t>(indices.size()) };

	return _meshes.insert(data);
}

Error RenderingServer::meshFree(Mesh mesh) {
	CHECK_IF_VALID(_meshes, mesh, "Mesh");
	MeshRS data = _meshes.remove(mesh).value();

	_pDevice->bufferDestroy(data.vertexBuffer);
	_pDevice->bufferDestroy(data.indexBuffer);

	return Error::None;
}

MeshInstance RenderingServer::meshInstanceCreate() {
	MeshInstanceRS data{};

	return _meshInstances.insert(data);
}

Error RenderingServer::meshInstanceSetMesh(MeshInstance meshInstance, Mesh mesh) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");

	if (!_meshes.has(mesh)) {
		return Error::InvalidParameter;
	}

	_meshInstances[meshInstance].mesh = mesh;

	return Error::None;
}

Error RenderingServer::meshInstanceSetTransform(
		MeshInstance meshInstance, const glm::mat4 &transform) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");
	_meshInstances[meshInstance].transform = transform;

	return Error::None;
}

Error RenderingServer::meshInstanceFree(MeshInstance meshInstance) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");
	_meshInstances.remove(meshInstance);

	return Error::None;
}

Light RenderingServer::lightCreate() {
	Light light = _lights.insert({});
	_updateLights();

	return light;
}

Error RenderingServer::lightSetPosition(Light light, const glm::vec3 &position) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].position = position;
	_updateLights();

	return Error::None;
}

Error RenderingServer::lightSetRange(Light light, float range) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].range = range;
	_updateLights();

	return Error::None;
}

Error RenderingServer::lightSetColor(Light light, const glm::vec3 &color) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].color = color;
	_updateLights();

	return Error::None;
}

Error RenderingServer::lightSetIntensity(Light light, float intensity) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].intensity = intensity;
	_updateLights();

	return Error::None;
}

Error RenderingServer::lightFree(Light light) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights.remove(light);
	_updateLights();
	return Error::None;
}

void RenderingServer::draw() {
	int width, height;
	SDL_Vulkan_GetDrawableSize(_pWindow, &width, &height);

	_pDevice->windowResize(width, height);

	float aspect = static_cast<float>(width) / static_cast<float>(height);

	glm::mat4 proj = _camera.projectionMatrix(aspect);
	glm::mat4 view = _camera.viewMatrix();

	_pDevice->updateUniformBuffer(proj, view, _lights.size());

	vk::CommandBuffer commandBuffer = _pDevice->drawBegin();

	for (const auto &[meshInstance, meshInstanceRS] : _meshInstances.map()) {
		const MeshRS &mesh = _meshes[meshInstanceRS.mesh];

		glm::mat4 modelView = meshInstanceRS.transform * view;

		MeshPushConstants constants{};
		constants.model = meshInstanceRS.transform;
		constants.modelViewNormal = glm::transpose(glm::inverse(modelView));

		commandBuffer.pushConstants(_pDevice->getPipelineLayout(), vk::ShaderStageFlagBits::eVertex,
				0, sizeof(MeshPushConstants), &constants);

		vk::DeviceSize offset = 0;
		commandBuffer.bindVertexBuffers(0, 1, &mesh.vertexBuffer.buffer, &offset);
		commandBuffer.bindIndexBuffer(mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
		commandBuffer.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
	}

	_pDevice->drawEnd(commandBuffer);
}

std::vector<const char *> getRequiredExtensions() {
	uint32_t extensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);

	std::vector<const char *> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

	return extensions;
}

RenderingServer::RenderingServer(SDL_Window *pWindow) {
	_pDevice = new RenderingDevice(getRequiredExtensions(), true);

	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(pWindow, _pDevice->getInstance(), &surface);

	int width, height;
	SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
	_pDevice->init(surface, width, height);

	_pWindow = pWindow;
}
