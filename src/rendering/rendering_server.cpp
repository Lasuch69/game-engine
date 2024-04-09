#include <cstdint>
#include <iostream>

#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>

#include "rendering_device.h"
#include "rendering_server.h"

#define CHECK_IF_VALID(owner, id, what)                                                            \
	if (!owner.has(id)) {                                                                          \
		std::cout << "ERROR: " << what << ": " << id << " is not valid resource!" << std::endl;    \
		return;                                                                                    \
	}

void RenderingServer::_updateLights() {
	if (_pointLights.size() == 0) {
		LightData lightData{};
		_pDevice->updateLightBuffer((uint8_t *)&lightData, sizeof(LightData));
		return;
	}

	std::vector<LightData> lights;
	for (const auto &[id, light] : _pointLights.map()) {
		if (lights.size() >= 8)
			break;

		lights.push_back(light);
	}

	_pDevice->updateLightBuffer((uint8_t *)lights.data(), sizeof(LightData) * lights.size());
}

void RS::cameraSetTransform(const glm::mat4 &transform) {
	_camera.transform = transform;
}

void RS::cameraSetFovY(float fovY) {
	_camera.fovY = fovY;
}

void RS::cameraSetZNear(float zNear) {
	_camera.zNear = zNear;
}

void RS::cameraSetZFar(float zFar) {
	_camera.zFar = zFar;
}

MeshID RS::meshCreate() {
	return _meshes.insert({});
}

void RS::meshAddPrimitive(
		MeshID mesh, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
	CHECK_IF_VALID(_meshes, mesh, "Mesh");

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

	Primitive primitive = {
		vertexBuffer,
		indexBuffer,
		static_cast<uint32_t>(indices.size()),
	};

	_meshes[mesh].primitives.push_back(primitive);
}

void RS::meshFree(MeshID mesh) {
	CHECK_IF_VALID(_meshes, mesh, "Mesh");

	Mesh data = _meshes.remove(mesh).value();

	for (Primitive primitive : data.primitives) {
		_pDevice->bufferDestroy(primitive.vertexBuffer);
		_pDevice->bufferDestroy(primitive.indexBuffer);
	}
}

MeshInstanceID RenderingServer::meshInstanceCreate() {
	return _meshInstances.insert({});
}

void RS::meshInstanceSetMesh(MeshInstanceID meshInstance, MeshID mesh) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");
	CHECK_IF_VALID(_meshes, mesh, "Mesh")

	_meshInstances[meshInstance].mesh = mesh;
}

void RS::meshInstanceSetTransform(MeshInstanceID meshInstance, const glm::mat4 &transform) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");
	_meshInstances[meshInstance].transform = transform;
}

void RS::meshInstanceFree(MeshInstanceID meshInstance) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");
	_meshInstances.remove(meshInstance);
}

PointLightID RenderingServer::pointLightCreate() {
	PointLightID pointLight = _pointLights.insert({});
	_updateLights();

	return pointLight;
}

void RS::pointLightSetPosition(PointLightID pointLight, const glm::vec3 &position) {
	CHECK_IF_VALID(_pointLights, pointLight, "PointLight");
	_pointLights[pointLight].position = position;
	_updateLights();
}

void RS::pointLightSetRange(PointLightID pointLight, float range) {
	CHECK_IF_VALID(_pointLights, pointLight, "PointLight");
	_pointLights[pointLight].range = range;
	_updateLights();
}

void RS::pointLightSetColor(PointLightID pointLight, const glm::vec3 &color) {
	CHECK_IF_VALID(_pointLights, pointLight, "PointLight");
	_pointLights[pointLight].color = color;
	_updateLights();
}

void RS::pointLightSetIntensity(PointLightID pointLight, float intensity) {
	CHECK_IF_VALID(_pointLights, pointLight, "PointLight");
	_pointLights[pointLight].intensity = intensity;
	_updateLights();
}

void RS::pointLightFree(PointLightID pointLight) {
	CHECK_IF_VALID(_pointLights, pointLight, "PointLight");
	_pointLights.remove(pointLight);
	_updateLights();
}

void RenderingServer::draw() {
	float aspect = static_cast<float>(_width) / static_cast<float>(_height);

	glm::mat4 proj = _camera.projectionMatrix(aspect);
	glm::mat4 view = _camera.viewMatrix();

	_pDevice->updateUniformBuffer(proj, view, _pointLights.size());

	vk::CommandBuffer commandBuffer = _pDevice->drawBegin();

	for (const auto &[meshInstance, meshInstanceRS] : _meshInstances.map()) {
		const Mesh &mesh = _meshes[meshInstanceRS.mesh];

		glm::mat4 modelView = meshInstanceRS.transform * view;

		MeshPushConstants constants{};
		constants.model = meshInstanceRS.transform;
		constants.modelViewNormal = glm::transpose(glm::inverse(modelView));

		commandBuffer.pushConstants(_pDevice->getPipelineLayout(), vk::ShaderStageFlagBits::eVertex,
				0, sizeof(MeshPushConstants), &constants);

		for (const Primitive &primitive : mesh.primitives) {
			vk::DeviceSize offset = 0;

			commandBuffer.bindVertexBuffers(0, 1, &primitive.vertexBuffer.buffer, &offset);
			commandBuffer.bindIndexBuffer(primitive.indexBuffer.buffer, 0, vk::IndexType::eUint32);
			commandBuffer.drawIndexed(primitive.indexCount, 1, 0, 0, 0);
		}
	}

	_pDevice->drawEnd(commandBuffer);
}

void RS::init(const std::vector<const char *> &extensions, bool validation) {
	_pDevice = new RenderingDevice(extensions, validation);
}

vk::Instance RS::getVkInstance() const {
	return _pDevice->getInstance();
}

void RS::windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
	_pDevice->init(surface, width, height);

	_width = width;
	_height = height;
}

void RS::windowResized(uint32_t width, uint32_t height) {
	_pDevice->windowResize(width, height);

	_width = width;
	_height = height;
}
