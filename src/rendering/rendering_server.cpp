#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>

#include <SDL2/SDL_vulkan.h>

#include <glm/glm.hpp>

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

Mesh RS::meshCreate(const std::vector<Primitive> &primitives) {
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
		Material material = primitive.material;

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

	vk::DeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
	AllocatedBuffer vertexBuffer = _pDevice->bufferCreate(
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			vertexBufferSize);

	_pDevice->bufferSend(vertexBuffer.buffer, (uint8_t *)vertices.data(), (size_t)vertexBufferSize);

	vk::DeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
	AllocatedBuffer indexBuffer = _pDevice->bufferCreate(
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			indexBufferSize);

	_pDevice->bufferSend(indexBuffer.buffer, (uint8_t *)indices.data(), (size_t)indexBufferSize);

	return _meshes.insert({
			vertexBuffer,
			indexBuffer,
			_primitives,
	});
}

void RS::meshFree(Mesh mesh) {
	std::optional<MeshRD> result = _meshes.remove(mesh);

	if (!result.has_value())
		return;

	MeshRD _mesh = result.value();
	_pDevice->bufferDestroy(_mesh.vertexBuffer);
	_pDevice->bufferDestroy(_mesh.indexBuffer);
}

MeshInstance RenderingServer::meshInstanceCreate() {
	return _meshInstances.insert({});
}

void RS::meshInstanceSetMesh(MeshInstance meshInstance, Mesh mesh) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");
	CHECK_IF_VALID(_meshes, mesh, "Mesh")

	_meshInstances[meshInstance].mesh = mesh;
}

void RS::meshInstanceSetTransform(MeshInstance meshInstance, const glm::mat4 &transform) {
	CHECK_IF_VALID(_meshInstances, meshInstance, "MeshInstance");

	_meshInstances[meshInstance].transform = transform;
}

void RS::meshInstanceFree(MeshInstance meshInstance) {
	_meshInstances.remove(meshInstance);
}

DirectionalLight RS::directionalLightCreate() {
	return _pDevice->getLightStorage().directionalLightCreate();
}

void RS::directionalLightSetDirection(
		DirectionalLight directionalLight, const glm::vec3 &direction) {
	_pDevice->getLightStorage().directionalLightSetDirection(directionalLight, direction);
}

void RS::directionalLightSetIntensity(DirectionalLight directionalLight, float intensity) {
	_pDevice->getLightStorage().directionalLightSetIntensity(directionalLight, intensity);
}

void RS::directionalLightSetColor(DirectionalLight directionalLight, const glm::vec3 &color) {
	_pDevice->getLightStorage().directionalLightSetColor(directionalLight, color);
}

void RS::directionalLightFree(DirectionalLight directionalLight) {
	_pDevice->getLightStorage().directionalLightFree(directionalLight);
}

PointLight RenderingServer::pointLightCreate() {
	return _pDevice->getLightStorage().pointLightCreate();
}

void RS::pointLightSetPosition(PointLight pointLight, const glm::vec3 &position) {
	_pDevice->getLightStorage().pointLightSetPosition(pointLight, position);
}

void RS::pointLightSetRange(PointLight pointLight, float range) {
	_pDevice->getLightStorage().pointLightSetRange(pointLight, range);
}

void RS::pointLightSetColor(PointLight pointLight, const glm::vec3 &color) {
	_pDevice->getLightStorage().pointLightSetColor(pointLight, color);
}

void RS::pointLightSetIntensity(PointLight pointLight, float intensity) {
	_pDevice->getLightStorage().pointLightSetIntensity(pointLight, intensity);
}

void RS::pointLightFree(PointLight pointLight) {
	_pDevice->getLightStorage().pointLightFree(pointLight);
}

Texture RS::textureCreate(const std::shared_ptr<Image> image) {
	if (image == nullptr)
		return NULL_HANDLE;

	TextureRD _texture = _pDevice->textureCreate(image);
	return _textures.insert(_texture);
}

void RS::textureFree(Texture texture) {
	std::optional<TextureRD> _texture = _textures.remove(texture);

	if (!_texture.has_value())
		return;

	_pDevice->textureDestroy(_texture.value());
}

Material RS::materialCreate(const MaterialInfo &info) {
	TextureRD albedo = _textures.get_id_or_else(info.albedo, _albedoFallback);
	TextureRD normal = _textures.get_id_or_else(info.normal, _normalFallback);
	TextureRD metallic = _textures.get_id_or_else(info.metallic, _metallicFallback);
	TextureRD roughness = _textures.get_id_or_else(info.roughness, _roughnessFallback);

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

	vk::DescriptorSetLayout textureLayout = _pDevice->getTextureLayout();

	vk::DescriptorSetAllocateInfo allocInfo = {};
	allocInfo.setDescriptorPool(_pDevice->getDescriptorPool());
	allocInfo.setDescriptorSetCount(1);
	allocInfo.setSetLayouts(textureLayout);

	VkDescriptorSet textureSet = _pDevice->getDevice().allocateDescriptorSets(allocInfo)[0];

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

	_pDevice->getDevice().updateDescriptorSets(writeInfos, nullptr);

	return _materials.insert({ textureSet });
}

void RS::materialFree(Material material) {
	_materials.remove(material);
}

void RenderingServer::draw() {
	_pDevice->updateUniformBuffer(_camera.transform[3]);

	float aspect = static_cast<float>(_width) / static_cast<float>(_height);
	glm::mat4 projView = _camera.projectionMatrix(aspect) * _camera.viewMatrix();

	vk::CommandBuffer commandBuffer = _pDevice->drawBegin();

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pDevice->getDepthPipeline());

	for (const auto &[_, meshInstance] : _meshInstances.map()) {
		const MeshRD &mesh = _meshes[meshInstance.mesh];

		vk::PipelineLayout pipelineLayout = _pDevice->getDepthPipelineLayout();

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

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _pDevice->getMaterialPipeline());

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			_pDevice->getMaterialPipelineLayout(), 0, _pDevice->getMaterialSets(), nullptr);

	for (const auto &[_, meshInstance] : _meshInstances.map()) {
		const MeshRD &mesh = _meshes[meshInstance.mesh];

		vk::PipelineLayout pipelineLayout = _pDevice->getMaterialPipelineLayout();

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
			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 2,
					material.textureSet, nullptr);

			commandBuffer.drawIndexed(primitive.indexCount, 1, primitive.firstIndex, 0, 0);
		}
	}

	_pDevice->drawEnd(commandBuffer);
}

vk::Instance RS::getVkInstance() const {
	return _pDevice->getInstance();
}

void RS::windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
	_pDevice->init(surface, width, height);

	_width = width;
	_height = height;

	{
		std::vector<uint8_t> data = { 255, 255, 255, 255 };
		std::shared_ptr<Image> albedo(new Image(1, 1, Image::Format::RGBA8, data));

		_albedoFallback = _pDevice->textureCreate(albedo);
	}

	{
		std::vector<uint8_t> data = { 127, 127 };
		std::shared_ptr<Image> normal(new Image(1, 1, Image::Format::RG8, data));

		_normalFallback = _pDevice->textureCreate(normal);
	}

	{
		std::vector<uint8_t> data = { 0 };
		std::shared_ptr<Image> metallic(new Image(1, 1, Image::Format::R8, data));

		_metallicFallback = _pDevice->textureCreate(metallic);
	}

	{
		std::vector<uint8_t> data = { 127 };
		std::shared_ptr<Image> roughness(new Image(1, 1, Image::Format::R8, data));

		_roughnessFallback = _pDevice->textureCreate(roughness);
	}
}

void RS::windowResized(uint32_t width, uint32_t height) {
	_pDevice->windowResize(width, height);

	_width = width;
	_height = height;
}

std::vector<const char *> getRequiredExtensions() {
	uint32_t extensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);

	std::vector<const char *> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

	return extensions;
}

void RS::initImGui() {
	_pDevice->initImGui();
}

void RS::initialize(int argc, char **argv) {
	bool useValidation = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp("--validation", argv[i]) == 0)
			useValidation = true;
	}

	_pDevice = new RenderingDevice(getRequiredExtensions(), useValidation);
}
