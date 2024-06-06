#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <glm/glm.hpp>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_vulkan.h>

#include <rendering/rendering_device.h>
#include <rendering/storage/mesh_storage.h>
#include <rendering/types/vertex.h>

#include <io/image.h>
#include <io/mesh.h>

#include "rendering_server.h"

void onSwapchainResized(vk::ImageView colorView) {
	SDL_Log("Swapchain resized!");
	return;
}

ObjectID RS::_meshCreate(const Mesh &mesh) {
	size_t vertexCount = 0;
	size_t indexCount = 0;

	for (uint32_t i = 0; i < mesh.primitiveCount; i++) {
		vertexCount += mesh.pPrimitives[i].vertices.count;
		indexCount += mesh.pPrimitives[i].indices.count;
	}

	Vertex *pVertices = static_cast<Vertex *>(malloc(sizeof(Vertex) * vertexCount));
	uint32_t *pIndices = static_cast<uint32_t *>(malloc(sizeof(uint32_t) * indexCount));

	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;

	uint32_t primitiveCount = mesh.primitiveCount;
	PrimitiveRD *pPrimitives =
			static_cast<PrimitiveRD *>(malloc(sizeof(PrimitiveRD) * primitiveCount));

	for (uint32_t i = 0; i < mesh.primitiveCount; i++) {
		PrimitiveRD primitive = {};
		primitive.firstIndex = indexOffset;
		primitive.indexCount = mesh.pPrimitives[i].indices.count;
		primitive.material = mesh.pPrimitives[i].materialIndex;

		pPrimitives[i] = primitive;

		for (uint32_t j = 0; j < mesh.pPrimitives[i].indices.count; j++) {
			pIndices[indexOffset] = vertexOffset + mesh.pPrimitives[i].indices.pData[j];
			indexOffset++;
		}

		size_t size = sizeof(Vertex) * mesh.pPrimitives[i].vertices.count;
		memcpy(&pVertices[vertexOffset], mesh.pPrimitives[i].vertices.pData, size);
		vertexOffset += mesh.pPrimitives[i].vertices.count;
	}

	AllocatedBuffer vertexBuffer = _renderingDevice.bufferCreate(
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			sizeof(Vertex) * vertexCount);

	AllocatedBuffer indexBuffer = _renderingDevice.bufferCreate(
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			sizeof(uint32_t) * indexCount);

	_renderingDevice.bufferSend(vertexBuffer.buffer, pVertices, sizeof(Vertex) * vertexCount);
	_renderingDevice.bufferSend(indexBuffer.buffer, pIndices, sizeof(uint32_t) * indexCount);

	MeshRD meshRD = {
		vertexBuffer,
		indexBuffer,
		pPrimitives,
		primitiveCount,
	};

	return _meshStorage.meshAppend(meshRD);
}

void RS::_meshDestroy(ObjectID meshID) {
	MeshRD meshRD = _meshStorage.meshRemove(meshID);

	_renderingDevice.bufferDestroy(meshRD.vertexBuffer);
	_renderingDevice.bufferDestroy(meshRD.indexBuffer);
	free(meshRD.pPrimitives);
}

ObjectID RS::_textureCreate(const Image *pImage) {
	if (pImage == nullptr)
		return 0;

	uint32_t width = pImage->getWidth();
	uint32_t height = pImage->getHeight();

	vk::Format format = vk::Format::eUndefined;

	switch (pImage->getFormat()) {
		case Image::Format::R8:
			format = vk::Format::eR8Unorm;
			break;
		case Image::Format::RG8:
			format = vk::Format::eR8G8Unorm;
			break;
		case Image::Format::RGB8:
			format = vk::Format::eR8G8B8Unorm;
			break;
		case Image::Format::RGBA8:
			format = vk::Format::eR8G8B8A8Unorm;
			break;
		case Image::Format::RGBA32F:
			format = vk::Format::eR32G32B32A32Sfloat;
			break;
	}

	AllocatedImage image = _renderingDevice.imageCreate(width, height, format, 1,
			vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

	_renderingDevice.imageLayoutTransition(image.image, format, 1, 1, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal);

	std::vector<uint8_t> data = pImage->getData();
	_renderingDevice.imageSend(image.image, width, height, data.data(), data.size(),
			vk::ImageLayout::eTransferDstOptimal);

	_renderingDevice.imageLayoutTransition(image.image, format, 1, 1,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

	vk::SamplerAddressMode repeatMode = vk::SamplerAddressMode::eRepeat;

	vk::ImageView imageView = _renderingDevice.imageViewCreate(image.image, format, 1);
	vk::Sampler sampler = _renderingDevice.samplerCreate(vk::Filter::eLinear, repeatMode, 1);

	TextureRD texture = {
		image,
		imageView,
		sampler,
	};

	return _textureStorage.textureAppend(texture);
}

void RS::_textureDestroy(ObjectID textureID) {
	TextureRD texture = _textureStorage.textureRemove(textureID);
	_renderingDevice.samplerDestroy(texture.sampler);
	_renderingDevice.imageViewDestroy(texture.imageView);
	_renderingDevice.imageDestroy(texture.image);
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

ObjectID RS::meshCreate(const Mesh &mesh) {
	return _meshCreate(mesh);
}

void RS::meshFree(ObjectID meshID) {
	_meshDestroy(meshID);
}

ObjectID RenderingServer::meshInstanceCreate() {}

void RS::meshInstanceSetMesh(ObjectID meshInstanceID, ObjectID meshID) {}

void RS::meshInstanceSetTransform(ObjectID meshInstanceID, const glm::mat4 &transform) {}

void RS::meshInstanceFree(ObjectID meshInstanceID) {}

ObjectID RS::lightCreate(LightType type) {
	return _lightStorage.lightCreate(type);
}

void RS::lightSetTransform(ObjectID lightID, const glm::mat4 &transform) {
	float pTransform[16];
	memcpy(pTransform, &transform, sizeof(float) * 16);
	_lightStorage.lightSetTransform(lightID, pTransform);
}

void RS::lightSetColor(ObjectID lightID, const glm::vec3 &color) {
	float pColor[3];
	memcpy(pColor, &color, sizeof(float) * 3);
	_lightStorage.lightSetColor(lightID, pColor);
}

void RS::lightSetIntensity(ObjectID lightID, float intensity) {
	_lightStorage.lightSetIntensity(lightID, intensity);
}

void RS::lightFree(ObjectID lightID) {
	_lightStorage.lightFree(lightID);
}

ObjectID RS::textureCreate(const Image *pImage) {
	return _textureCreate(pImage);
}

void RS::textureDestroy(ObjectID textureID) {
	_textureDestroy(textureID);
}

void RenderingServer::draw() {
	vk::CommandBuffer commandBuffer = _renderingDevice.drawBegin();
	commandBuffer.nextSubpass(vk::SubpassContents::eInline);
	commandBuffer.nextSubpass(vk::SubpassContents::eInline);

	_tonemapEffect.draw(commandBuffer, 1.25, 8.0);

	_renderingDevice.drawEnd(commandBuffer);
}

void RS::windowInit(SDL_Window *pWindow) {
	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(pWindow, _renderingDevice.getInstance(), nullptr, &surface);

	int width, height;
	SDL_GetWindowSizeInPixels(pWindow, &width, &height);
	_renderingDevice.windowInit(surface, width, height);
	_renderingDevice.setOnSwapchainResizedCallback(&onSwapchainResized);

	vk::Device device = _renderingDevice.getDevice();
	vk::DescriptorPool descriptorPool = _renderingDevice.getDescriptorPool();
	vk::RenderPass renderPass = _renderingDevice.getRenderPass();

	_skyEffect.create(device, descriptorPool, renderPass);
	_tonemapEffect.create(device, descriptorPool, renderPass);
	_tonemapEffect.updateInput(device, _renderingDevice.getColorView());
}

void RS::windowResized(uint32_t width, uint32_t height) {
	_renderingDevice.windowResize(width, height);
}

void RS::initialize(int argc, char **argv) {
	bool validation = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp("--validation", argv[i]) == 0)
			validation = true;
	}

	_renderingDevice.init(validation);
}
