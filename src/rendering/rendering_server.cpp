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
	std::optional<TextureRD> result = _textures.remove(texture);

	if (!result.has_value())
		return;

	TextureRD _texture = result.value();
	_pDevice->imageDestroy(_texture.image);
	_pDevice->imageViewDestroy(_texture.imageView);
	_pDevice->samplerDestroy(_texture.sampler);
}

Material RS::materialCreate(Texture albedo, Texture normal, Texture roughness) {
	if (!_textures.has(albedo))
		albedo = _whiteTexture;

	TextureRD _albedo = _textures[albedo];

	if (!_textures.has(normal))
		normal = _whiteTexture;

	TextureRD _normal = _textures[normal];

	if (!_textures.has(roughness))
		roughness = _whiteTexture;

	TextureRD _roughness = _textures[roughness];

	vk::DescriptorSetLayout textureLayout = _pDevice->getTextureLayout();

	vk::DescriptorSetAllocateInfo allocInfo =
			vk::DescriptorSetAllocateInfo()
					.setDescriptorPool(_pDevice->getDescriptorPool())
					.setDescriptorSetCount(1)
					.setSetLayouts(textureLayout);

	VkDescriptorSet textureSet = _pDevice->getDevice().allocateDescriptorSets(allocInfo)[0];

	std::array<vk::DescriptorImageInfo, 3> imageInfos = {
		vk::DescriptorImageInfo()
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(_albedo.imageView)
				.setSampler(_albedo.sampler),
		vk::DescriptorImageInfo()
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(_normal.imageView)
				.setSampler(_normal.sampler),
		vk::DescriptorImageInfo()
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(_roughness.imageView)
				.setSampler(_roughness.sampler),
	};

	std::array<vk::WriteDescriptorSet, 3> writeInfos = {
		vk::WriteDescriptorSet()
				.setDstSet(textureSet)
				.setDstBinding(0)
				.setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount(1)
				.setImageInfo(imageInfos[0]),
		vk::WriteDescriptorSet()
				.setDstSet(textureSet)
				.setDstBinding(1)
				.setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount(1)
				.setImageInfo(imageInfos[1]),
		vk::WriteDescriptorSet()
				.setDstSet(textureSet)
				.setDstBinding(2)
				.setDstArrayElement(0)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount(1)
				.setImageInfo(imageInfos[2]),
	};

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

	std::shared_ptr<Image> whiteImage(
			new Image(1, 1, Image::Format::RGBA8, { 255, 255, 255, 255 }));

	// create fallback white texture
	_whiteTexture = textureCreate(whiteImage);
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

void RS::initialize(int argc, char **argv) {
	bool useValidation = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp("--validation", argv[i]) == 0)
			useValidation = true;
	}

	_pDevice = new RenderingDevice(getRequiredExtensions(), useValidation);
}
