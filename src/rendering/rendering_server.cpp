#include <cstdint>
#include <iostream>
#include <memory>

#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>

#include "../image.h"

#include "render_scene_forward.h"
#include "storage/light_storage.h"

#include "rendering_device.h"
#include "rendering_server.h"

#define CHECK_IF_VALID(owner, id, what)                                                            \
	if (!owner.has(id)) {                                                                          \
		std::cout << "ERROR: " << what << ": " << id << " is not valid resource!" << std::endl;    \
		return;                                                                                    \
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

ObjectID RS::meshCreate(const std::vector<Primitive> &primitives) {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	{
		size_t totalVertexCount = 0;
		size_t totalIndexCount = 0;

		for (const Primitive &primitive : primitives) {
			totalVertexCount += primitive.vertices.size();
			totalIndexCount += primitive.indices.size();
		}

		vertices.resize(totalVertexCount);
		indices.resize(totalIndexCount);
	}

	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;

	std::vector<PrimitiveRD> _primitives = {};

	for (const Primitive &primitive : primitives) {
		uint32_t indexCount = static_cast<uint32_t>(primitive.indices.size());
		uint32_t firstIndex = indexOffset;
		ObjectID material = primitive.material;

		_primitives.push_back({
				indexCount,
				firstIndex,
				material,
		});

		for (uint32_t index : primitive.indices) {
			indices[indexOffset] = vertexOffset + index;
			indexOffset++;
		}

		Vertex *pDst = vertices.data();
		const Vertex *pSrc = primitive.vertices.data();
		size_t vertexCount = primitive.vertices.size();

		memcpy(&pDst[vertexOffset], pSrc, sizeof(Vertex) * vertexCount);

		vertexOffset += vertexCount;
	}

	RD &rd = RD::getSingleton();

	vk::DeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
	AllocatedBuffer vertexBuffer = rd.bufferCreate(
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vertexBufferSize);

	rd.bufferSend(vertexBuffer.buffer, (uint8_t *)vertices.data(), (size_t)vertexBufferSize);

	vk::DeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
	AllocatedBuffer indexBuffer = rd.bufferCreate(
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			indexBufferSize);

	rd.bufferSend(indexBuffer.buffer, (uint8_t *)indices.data(), (size_t)indexBufferSize);

	return _meshes.insert({
			vertexBuffer,
			indexBuffer,
			_primitives,
	});
}

void RS::meshFree(ObjectID mesh) {
	_meshes.free(mesh);
}

ObjectID RenderingServer::meshInstanceCreate() {
	return _meshInstances.insert({});
}

void RS::meshInstanceSetMesh(ObjectID meshInstance, ObjectID mesh) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");
	CHECK_IF_VALID(_meshes, mesh, "Mesh")

	_meshInstances[meshInstance].mesh = mesh;
}

void RS::meshInstanceSetTransform(ObjectID meshInstance, const glm::mat4 &transform) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");

	_meshInstances[meshInstance].transform = transform;
}

void RS::meshInstanceFree(ObjectID meshInstance) {
	_meshInstances.free(meshInstance);
}

ObjectID RS::lightCreate(LightType type) {
	return LS::getSingleton().lightCreate(type);
}

void RS::lightSetTransform(ObjectID light, const glm::mat4 &transform) {
	LS::getSingleton().lightSetTransform(light, reinterpret_cast<const float *>(&transform));
}

void RS::lightSetRange(ObjectID light, float range) {
	LS::getSingleton().lightSetRange(light, range);
}

void RS::lightSetColor(ObjectID light, const glm::vec3 &color) {
	LS::getSingleton().lightSetColor(light, reinterpret_cast<const float *>(&color));
}

void RS::lightSetIntensity(ObjectID light, float intensity) {
	LS::getSingleton().lightSetIntensity(light, intensity);
}

void RS::lightFree(ObjectID light) {
	LS::getSingleton().lightFree(light);
}

ObjectID RS::textureCreate(const std::shared_ptr<Image> image) {
	if (image == nullptr)
		return NULL_HANDLE;

	return _textures.insert({});
}

void RS::textureFree(ObjectID texture) {
	_textures.free(texture);
}

ObjectID RS::materialCreate(const MaterialInfo &info) {
	return _materials.insert({});
}

void RS::materialFree(ObjectID material) {
	_materials.free(material);
}

void RS::setExposure(float exposure) {}

void RS::setWhite(float white) {}

void RS::environmentSkyUpdate(const std::shared_ptr<Image> image) {}

void RenderingServer::draw() {
	/*
	RD &rd = RD::getSingleton();

	vk::CommandBuffer commandBuffer = rd.drawBegin();

	vk::ClearValue clearValue;
	clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);

	vk::Extent2D extent = rd.getSwapchainExtent();

	vk::Viewport viewport;
	viewport.setX(0.0f);
	viewport.setY(0.0f);
	viewport.setWidth(extent.width);
	viewport.setHeight(extent.height);
	viewport.setMinDepth(0.0f);
	viewport.setMaxDepth(1.0f);

	vk::Rect2D scissor;
	scissor.setOffset({ 0, 0 });
	scissor.setExtent(extent);

	vk::Rect2D renderArea;
	renderArea.setOffset({ 0, 0 });
	renderArea.setExtent(extent);

	vk::RenderPassBeginInfo renderPassInfo;
	renderPassInfo.setRenderPass(rd.getRenderPass());
	renderPassInfo.setFramebuffer(rd.getFramebuffer());
	renderPassInfo.setRenderArea(renderArea);
	renderPassInfo.setClearValues(clearValue);

	commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

	commandBuffer.setViewport(0, viewport);
	commandBuffer.setScissor(0, scissor);

	commandBuffer.endRenderPass();

	/*
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rd.getDepthPipeline());

	for (const auto &[_, meshInstance] : _meshInstances.map()) {
		const MeshRD &mesh = _meshes[meshInstance.mesh];

		vk::PipelineLayout pipelineLayout = rd.getDepthPipelineLayout();

		vk::DeviceSize offset = 0;
		commandBuffer.bindVertexBuffers(0, 1, &mesh.vertexBuffer.buffer, &offset);
		commandBuffer.bindIndexBuffer(mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);

		MeshPushConstants constants{};
		constants.projView = projView;
		constants.model = meshInstance.transform;

		commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0,
				sizeof(MeshPushConstants), &constants);

		for (const PrimitiveRD &primitive : mesh.primitives) {
			commandBuffer.drawIndexed(primitive.indexCount, 1, primitive.firstIndex, 0, 0);
		}
	}

	commandBuffer.nextSubpass(vk::SubpassContents::eInline);

	{
		// sky

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rd.getSkyPipeline());
		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
				rd.getSkyPipelineLayout(), 0, rd.getSkySet(), nullptr);

		SkyConstants constants{};
		constants.invProj = invProj;
		constants.invView = invView;

		commandBuffer.pushConstants(rd.getSkyPipelineLayout(), vk::ShaderStageFlagBits::eFragment,
				0, sizeof(constants), &constants);
		commandBuffer.draw(3, 1, 0, 0);
	}

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, rd.getMaterialPipeline());
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			rd.getMaterialPipelineLayout(), 0, rd.getMaterialSets(), nullptr);

	for (const auto &[_, meshInstance] : _meshInstances.map()) {
		const MeshRD &mesh = _meshes[meshInstance.mesh];

		vk::PipelineLayout pipelineLayout = rd.getMaterialPipelineLayout();

		vk::DeviceSize offset = 0;
		commandBuffer.bindVertexBuffers(0, 1, &mesh.vertexBuffer.buffer, &offset);
		commandBuffer.bindIndexBuffer(mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);

		MeshPushConstants constants{};
		constants.projView = projView;
		constants.model = meshInstance.transform;

		commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0,
				sizeof(MeshPushConstants), &constants);

		for (const PrimitiveRD &primitive : mesh.primitives) {
			MaterialRD material = _materials[primitive.material];
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 3,
					material.textureSet, nullptr);

			commandBuffer.drawIndexed(primitive.indexCount, 1, primitive.firstIndex, 0, 0);
		}
	}
	*/

	RenderSceneForward::getSingleton().draw();
	// rd.drawEnd(commandBuffer);
}

vk::Instance RS::getVkInstance() const {
	return RD::getSingleton().getInstance();
}

void RS::windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
	RD &rd = RD::getSingleton();
	rd.windowInit(surface, width, height);

	RenderSceneForward &scene = RenderSceneForward::getSingleton();
	scene.init();
}

void RS::windowResized(uint32_t width, uint32_t height) {
	RD::getSingleton().windowResize(width, height);
}

std::vector<const char *> getRequiredExtensions() {
	uint32_t extensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);

	std::vector<const char *> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

	return extensions;
}

void RS::initImGui() {
	RD::getSingleton().initImGui();
}

void RS::initialize(int argc, char **argv) {
	bool useValidation = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp("--validation", argv[i]) == 0)
			useValidation = true;
	}

	std::vector<const char *> extensions = getRequiredExtensions();
	RD::getSingleton().init(extensions, useValidation);
}
