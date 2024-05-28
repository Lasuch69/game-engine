#include <cstdint>
#include <iostream>
#include <memory>

#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>

#include "../image.h"

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
	return RD::getSingleton().getLightStorage().lightCreate(type);
}

void RS::lightSetTransform(ObjectID light, const glm::mat4 &transform) {
	RD::getSingleton().getLightStorage().lightSetTransform(light, transform);
}

void RS::lightSetRange(ObjectID light, float range) {
	RD::getSingleton().getLightStorage().lightSetRange(light, range);
}

void RS::lightSetColor(ObjectID light, const glm::vec3 &color) {
	RD::getSingleton().getLightStorage().lightSetColor(light, color);
}

void RS::lightSetIntensity(ObjectID light, float intensity) {
	RD::getSingleton().getLightStorage().lightSetIntensity(light, intensity);
}

void RS::lightFree(ObjectID light) {
	RD::getSingleton().getLightStorage().lightFree(light);
}

ObjectID RS::textureCreate(const std::shared_ptr<Image> image) {
	if (image == nullptr)
		return NULL_HANDLE;

	TextureRD _texture = RD::getSingleton().textureCreate(image);
	return _textures.insert(_texture);
}

void RS::textureFree(ObjectID texture) {
	_textures.free(texture);
}

ObjectID RS::materialCreate(const MaterialInfo &info) {
	TextureRD albedo = _textures.get_id_or_else(info.albedo, _albedoFallback);
	TextureRD normal = _textures.get_id_or_else(info.normal, _normalFallback);
	TextureRD metallic = _textures.get_id_or_else(info.metallic, _metallicFallback);
	TextureRD roughness = _textures.get_id_or_else(info.roughness, _roughnessFallback);

	RD &rd = RD::getSingleton();
	vk::Device device = rd.getDevice();
	vk::DescriptorPool descriptorPool = rd.getDescriptorPool();

	std::array<vk::DescriptorImageInfo, 4> imageInfos = {};
	imageInfos[0].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfos[0].setImageView(albedo.imageView);
	imageInfos[0].setSampler(albedo.sampler);

	imageInfos[1].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfos[1].setImageView(normal.imageView);
	imageInfos[1].setSampler(normal.sampler);

	imageInfos[2].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfos[2].setImageView(metallic.imageView);
	imageInfos[2].setSampler(metallic.sampler);

	imageInfos[3].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	imageInfos[3].setImageView(roughness.imageView);
	imageInfos[3].setSampler(roughness.sampler);

	vk::DescriptorSetLayout textureLayout = rd.getTextureLayout();

	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.setDescriptorPool(descriptorPool);
	allocInfo.setDescriptorSetCount(1);
	allocInfo.setSetLayouts(textureLayout);

	VkDescriptorSet textureSet = device.allocateDescriptorSets(allocInfo)[0];

	std::array<vk::WriteDescriptorSet, 4> writeInfos = {};
	writeInfos[0].setDstSet(textureSet);
	writeInfos[0].setDstBinding(0);
	writeInfos[0].setDstArrayElement(0);
	writeInfos[0].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeInfos[0].setDescriptorCount(1);
	writeInfos[0].setImageInfo(imageInfos[0]);

	writeInfos[1].setDstSet(textureSet);
	writeInfos[1].setDstBinding(1);
	writeInfos[1].setDstArrayElement(0);
	writeInfos[1].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeInfos[1].setDescriptorCount(1);
	writeInfos[1].setImageInfo(imageInfos[1]);

	writeInfos[2].setDstSet(textureSet);
	writeInfos[2].setDstBinding(2);
	writeInfos[2].setDstArrayElement(0);
	writeInfos[2].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeInfos[2].setDescriptorCount(1);
	writeInfos[2].setImageInfo(imageInfos[2]);

	writeInfos[3].setDstSet(textureSet);
	writeInfos[3].setDstBinding(3);
	writeInfos[3].setDstArrayElement(0);
	writeInfos[3].setDescriptorType(vk::DescriptorType::eCombinedImageSampler);
	writeInfos[3].setDescriptorCount(1);
	writeInfos[3].setImageInfo(imageInfos[3]);

	device.updateDescriptorSets(writeInfos, nullptr);

	return _materials.insert({ textureSet });
}

void RS::materialFree(ObjectID material) {
	_materials.free(material);
}

void RS::setExposure(float exposure) {
	RD::getSingleton().setExposure(exposure);
}

void RS::setWhite(float white) {
	RD::getSingleton().setWhite(white);
}

void RS::environmentSkyUpdate(const std::shared_ptr<Image> image) {
	RD::getSingleton().environmentSkyUpdate(image);
}

void RenderingServer::draw() {
	RD &rd = RD::getSingleton();
	rd.updateUniformBuffer(_camera.transform[3]);

	vk::Extent2D extent = rd.getSwapchainExtent();
	float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

	glm::mat4 proj = _camera.projectionMatrix(aspect);
	glm::mat4 view = _camera.viewMatrix();

	glm::mat4 invProj = glm::inverse(proj);
	glm::mat4 invView = glm::inverse(view);

	glm::mat4 projView = proj * view;

	vk::CommandBuffer commandBuffer = rd.drawBegin();

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

	rd.drawEnd(commandBuffer);
}

vk::Instance RS::getVkInstance() const {
	return RD::getSingleton().getInstance();
}

void RS::windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
	RD &rd = RD::getSingleton();
	rd.windowInit(surface, width, height);

	{
		std::vector<uint8_t> data = { 255, 255, 255, 255 };
		std::shared_ptr<Image> albedo(new Image(1, 1, Image::Format::RGBA8, data));

		_albedoFallback = rd.textureCreate(albedo);
	}

	{
		std::vector<uint8_t> data = { 127, 127 };
		std::shared_ptr<Image> normal(new Image(1, 1, Image::Format::RG8, data));

		_normalFallback = rd.textureCreate(normal);
	}

	{
		std::vector<uint8_t> data = { 0 };
		std::shared_ptr<Image> metallic(new Image(1, 1, Image::Format::R8, data));

		_metallicFallback = rd.textureCreate(metallic);
	}

	{
		std::vector<uint8_t> data = { 127 };
		std::shared_ptr<Image> roughness(new Image(1, 1, Image::Format::R8, data));

		_roughnessFallback = rd.textureCreate(roughness);
	}
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
