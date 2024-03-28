#include <cstdint>

#include <SDL2/SDL_vulkan.h>

#include <glm/glm.hpp>

#include "rendering_device.h"
#include "rendering_server.h"

#define CHECK_ID(owner, id, msg)                                                                   \
	if (!owner.has(id)) {                                                                          \
		printf("ERROR: %s ID: %ld is invalid.\n", msg, id);                                        \
		return;                                                                                    \
	}

void RS::_updateLights() {
	if (_lights.size() == 0) {
		LightData light{};
		_pDevice->updateLightBuffer(&light, 1);
		return;
	}

	std::vector<LightData> lights;

	for (const auto &[id, light] : _lights.map()) {
		lights.push_back(light);
	}

	_pDevice->updateLightBuffer(lights.data(), lights.size());
}

void RS::cameraSetTransform(const glm::mat4 &transform) { _camera.transform = transform; }

void RS::cameraSetFovY(float fovY) { _camera.fovY = fovY; }

void RS::cameraSetZNear(float zNear) { _camera.zNear = zNear; }

void RS::cameraSetZFar(float zFar) { _camera.zFar = zFar; }

MeshID RS::meshCreate(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
	Mesh mesh = _pDevice->meshCreate(vertices, indices);
	return _meshes.insert(mesh);
}

void RS::meshFree(MeshID meshID) {
	CHECK_ID(_meshes, meshID, "Could not free mesh!");
	Mesh mesh = _meshes.remove(meshID).value();
	_pDevice->meshDestroy(mesh);
}

LightID RS::lightCreate() {
	LightData light{};

	LightID lightID = _lights.insert(light);
	_updateLights();

	return lightID;
}

void RS::lightSetPosition(LightID lightID, const glm::vec3 &position) {
	CHECK_ID(_lights, lightID, "Could not set light position!");
	_lights[lightID].position = position;
	_updateLights();
}

void RS::lightSetRange(LightID lightID, float range) {
	CHECK_ID(_lights, lightID, "Could not set light range!");
	_lights[lightID].range = range;
	_updateLights();
}

void RS::lightSetColor(LightID lightID, const glm::vec3 &color) {
	CHECK_ID(_lights, lightID, "Could not set light color!");
	_lights[lightID].color = color;
	_updateLights();
}

void RS::lightSetIntensity(LightID lightID, float intensity) {
	CHECK_ID(_lights, lightID, "Could not set light intensity!");
	_lights[lightID].intensity = intensity;
	_updateLights();
}

void RS::lightFree(LightID lightID) {
	CHECK_ID(_lights, lightID, "Could not free light!");
	_lights.remove(lightID);
	_updateLights();
}

void RS::drawMesh(MeshID meshID, const glm::mat4 &transform) {
	CHECK_ID(_meshes, meshID, "Could not draw mesh!")
	_drawQueue.push_back(std::tuple<MeshID, glm::mat4>(meshID, transform));
}

void RS::submit() {
	int width, height;
	SDL_Vulkan_GetDrawableSize(_pWindow, &width, &height);

	_pDevice->windowResize(width, height);

	float aspect = static_cast<float>(width) / static_cast<float>(height);

	glm::mat4 proj = _camera.projectionMatrix(aspect);
	glm::mat4 view = _camera.viewMatrix();

	_pDevice->updateUniformBuffer(proj, view, _lights.size());

	vk::CommandBuffer commandBuffer = _pDevice->drawBegin();

	for (const auto &[meshID, transform] : _drawQueue) {
		const Mesh &mesh = _meshes[meshID];

		glm::mat4 modelView = transform * view;

		MeshPushConstants constants{};
		constants.model = transform;
		constants.modelViewNormal = glm::transpose(glm::inverse(modelView));

		commandBuffer.pushConstants(_pDevice->getPipelineLayout(), vk::ShaderStageFlagBits::eVertex,
				0, sizeof(MeshPushConstants), &constants);

		vk::DeviceSize offset = 0;
		commandBuffer.bindVertexBuffers(0, 1, &mesh.vertexBuffer.buffer, &offset);
		commandBuffer.bindIndexBuffer(mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
		commandBuffer.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
	}

	_pDevice->drawEnd(commandBuffer);

	_drawQueue.clear();
}

RS::RenderingServer(SDL_Window *pWindow) {
	_pDevice = new RenderingDevice(true);

	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(pWindow, _pDevice->getInstance(), &surface);

	int width, height;
	SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
	_pDevice->init(surface, width, height);

	_pWindow = pWindow;
}
