#include <cstdint>
#include <cstdio>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/matrix.hpp>

#include <vulkan/vulkan_core.h>

#include "shaders/material.gen.h"
#include "shaders/tonemap.gen.h"

#include "rendering_device.h"

vk::ShaderModule createShaderModule(vk::Device device, const uint32_t *pCode, size_t size) {
	vk::ShaderModuleCreateInfo createInfo =
			vk::ShaderModuleCreateInfo().setPCode(pCode).setCodeSize(size);

	return device.createShaderModule(createInfo);
}

AllocatedBuffer createBuffer(VmaAllocator allocator, vk::DeviceSize size,
		vk::BufferUsageFlags usage, VmaAllocationInfo &allocInfo) {
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = static_cast<VkBufferUsageFlags>(usage);

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkBuffer buffer;
	VmaAllocation allocation;
	vmaCreateBuffer(allocator, &createInfo, &allocCreateInfo, &buffer, &allocation, &allocInfo);

	return AllocatedBuffer{ allocation, buffer };
}

void updateInputAttachment(vk::Device device, vk::ImageView imageView, vk::DescriptorSet dstSet) {
	vk::DescriptorImageInfo imageInfo =
			vk::DescriptorImageInfo()
					.setImageView(imageView)
					.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
					.setSampler(VK_NULL_HANDLE);

	vk::WriteDescriptorSet writeInfo =
			vk::WriteDescriptorSet()
					.setDstSet(dstSet)
					.setDstBinding(0)
					.setDstArrayElement(0)
					.setDescriptorType(vk::DescriptorType::eInputAttachment)
					.setDescriptorCount(1)
					.setImageInfo(imageInfo);

	device.updateDescriptorSets(writeInfo, nullptr);
}

vk::Pipeline createPipeline(vk::Device device, vk::ShaderModule vertexStage,
		vk::ShaderModule fragmentStage, vk::PipelineLayout pipelineLayout,
		vk::RenderPass renderPass, uint32_t subpass,
		vk::PipelineVertexInputStateCreateInfo vertexInput) {
	vk::PipelineShaderStageCreateInfo vertexStageInfo =
			vk::PipelineShaderStageCreateInfo()
					.setModule(vertexStage)
					.setStage(vk::ShaderStageFlagBits::eVertex)
					.setPName("main");

	vk::PipelineShaderStageCreateInfo fragmentStageInfo =
			vk::PipelineShaderStageCreateInfo()
					.setModule(fragmentStage)
					.setStage(vk::ShaderStageFlagBits::eFragment)
					.setPName("main");

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertexStageInfo, fragmentStageInfo };

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly =
			vk::PipelineInputAssemblyStateCreateInfo().setTopology(
					vk::PrimitiveTopology::eTriangleList);

	vk::PipelineViewportStateCreateInfo viewportState =
			vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1);

	vk::PipelineRasterizationStateCreateInfo rasterizer =
			vk::PipelineRasterizationStateCreateInfo()
					.setDepthBiasEnable(VK_FALSE)
					.setRasterizerDiscardEnable(VK_FALSE)
					.setPolygonMode(vk::PolygonMode::eFill)
					.setLineWidth(1.0f)
					.setCullMode(vk::CullModeFlagBits::eBack)
					.setFrontFace(vk::FrontFace::eCounterClockwise)
					.setDepthBiasEnable(VK_FALSE);

	vk::PipelineMultisampleStateCreateInfo multisampling =
			vk::PipelineMultisampleStateCreateInfo()
					.setSampleShadingEnable(VK_FALSE)
					.setRasterizationSamples(vk::SampleCountFlagBits::e1);

	vk::PipelineDepthStencilStateCreateInfo depthStencil =
			vk::PipelineDepthStencilStateCreateInfo()
					.setDepthTestEnable(VK_TRUE)
					.setDepthWriteEnable(VK_TRUE)
					.setDepthCompareOp(vk::CompareOp::eLess)
					.setDepthBoundsTestEnable(VK_FALSE)
					.setStencilTestEnable(VK_FALSE);

	vk::PipelineColorBlendAttachmentState colorBlendAttachment =
			vk::PipelineColorBlendAttachmentState()
					.setColorWriteMask(vk::ColorComponentFlagBits::eR |
							vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
							vk::ColorComponentFlagBits::eA)
					.setBlendEnable(VK_FALSE);

	vk::PipelineColorBlendStateCreateInfo colorBlending =
			vk::PipelineColorBlendStateCreateInfo()
					.setLogicOpEnable(VK_FALSE)
					.setLogicOp(vk::LogicOp::eCopy)
					.setAttachments(colorBlendAttachment)
					.setBlendConstants({ 0.0f, 0.0f, 0.0f, 0.0f });

	std::vector<vk::DynamicState> dynamicStates = { vk::DynamicState::eViewport,
		vk::DynamicState::eScissor };

	vk::PipelineDynamicStateCreateInfo dynamicState =
			vk::PipelineDynamicStateCreateInfo().setDynamicStates(dynamicStates);

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo =
			vk::GraphicsPipelineCreateInfo()
					.setStages(shaderStages)
					.setPVertexInputState(&vertexInput)
					.setPInputAssemblyState(&inputAssembly)
					.setPViewportState(&viewportState)
					.setPRasterizationState(&rasterizer)
					.setPMultisampleState(&multisampling)
					.setPDepthStencilState(&depthStencil)
					.setPColorBlendState(&colorBlending)
					.setPDynamicState(&dynamicState)
					.setLayout(pipelineLayout)
					.setRenderPass(renderPass)
					.setSubpass(subpass);

	vk::ResultValue<vk::Pipeline> result =
			device.createGraphicsPipeline(VK_NULL_HANDLE, pipelineCreateInfo);

	if (result.result != vk::Result::eSuccess)
		printf("Failed to create pipeline!\n");

	return result.value;
}

vk::CommandBuffer RenderingDevice::_beginSingleTimeCommands() {
	vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo()
													  .setLevel(vk::CommandBufferLevel::ePrimary)
													  .setCommandPool(_pContext->commandPool)
													  .setCommandBufferCount(1);

	vk::CommandBuffer commandBuffer = _pContext->device.allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

void RenderingDevice::_endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
	commandBuffer.end();

	vk::SubmitInfo submitInfo =
			vk::SubmitInfo().setCommandBufferCount(1).setCommandBuffers(commandBuffer);

	_pContext->graphicsQueue.submit(submitInfo, VK_NULL_HANDLE);
	_pContext->graphicsQueue.waitIdle();

	_pContext->device.freeCommandBuffers(_pContext->commandPool, commandBuffer);
}

void RenderingDevice::_copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
	vk::CommandBuffer commandBuffer = _beginSingleTimeCommands();

	vk::BufferCopy bufferCopy = vk::BufferCopy().setSrcOffset(0).setDstOffset(0).setSize(size);
	commandBuffer.copyBuffer(srcBuffer, dstBuffer, bufferCopy);

	_endSingleTimeCommands(commandBuffer);
}

void RenderingDevice::_sendToBuffer(vk::Buffer dstBuffer, uint8_t *data, size_t size) {
	VmaAllocationInfo stagingAllocInfo;
	AllocatedBuffer stagingBuffer =
			createBuffer(_allocator, size, vk::BufferUsageFlagBits::eTransferSrc, stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, data, size);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	_copyBuffer(stagingBuffer.buffer, dstBuffer, size);

	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

Mesh RenderingDevice::_uploadMesh(
		const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;

	{
		vk::DeviceSize bufferSize = sizeof(Vertex) * vertices.size();

		VmaAllocationInfo allocInfo;
		vertexBuffer = createBuffer(_allocator, bufferSize,
				vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
				allocInfo);

		_sendToBuffer(vertexBuffer.buffer, (uint8_t *)vertices.data(), (size_t)bufferSize);
	}

	{
		vk::DeviceSize bufferSize = sizeof(uint32_t) * indices.size();

		VmaAllocationInfo allocInfo;
		indexBuffer = createBuffer(_allocator, bufferSize,
				vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
				allocInfo);

		_sendToBuffer(indexBuffer.buffer, (uint8_t *)indices.data(), (size_t)bufferSize);
	}

	return Mesh{ vertexBuffer, indexBuffer, static_cast<uint32_t>(indices.size()) };
}

void RenderingDevice::_updateUniformBuffer(const Camera *pCamera) {
	vk::Extent2D extent = _pContext->swapchainExtent;
	float aspect = (float)extent.width / (float)extent.height;

	glm::mat4 view = pCamera->viewMatrix();
	glm::mat4 projView = pCamera->projectionMatrix(aspect) * view;

	UniformBufferObject ubo = { projView, view, static_cast<uint32_t>(_lights.size()) };
	memcpy(_uniformAllocInfos[_frame].pMappedData, &ubo, sizeof(ubo));
}

void RenderingDevice::createMesh(const std::vector<Vertex> &vertices,
		const std::vector<uint32_t> &indices, const glm::mat4 &transform) {
	Mesh mesh = _uploadMesh(vertices, indices);
	mesh.transform = transform;

	_meshes.push_back(mesh);
}

void RenderingDevice::createLight(
		const glm::vec3 &position, const glm::vec3 &color, float intensity) {
	if (_lights.size() >= MAX_LIGHT_COUNT) {
		printf("Light limit of: %d exceeded!\n", MAX_LIGHT_COUNT);
		return;
	}

	_lights.push_back(LightData{ position, 0.0f, color, intensity });

	_sendToBuffer(
			_lightBuffer.buffer, (uint8_t *)_lights.data(), sizeof(LightData) * MAX_LIGHT_COUNT);
}

void RenderingDevice::createCamera(const glm::mat4 transform, float fovY, float zNear, float zFar) {
	if (_pCamera != nullptr)
		free(_pCamera);

	Camera *pCamera = new Camera();
	pCamera->transform = transform;
	pCamera->fovY = fovY;
	pCamera->zNear = zNear;
	pCamera->zFar = zFar;

	_pCamera = pCamera;
}

void RenderingDevice::draw() {
	vk::Device device = _pContext->device;
	vk::CommandBuffer commandBuffer = _commandBuffers[_frame];

	vk::Result result = device.waitForFences(_fences[_frame], VK_TRUE, UINT64_MAX);

	if (result != vk::Result::eSuccess) {
		printf("Device failed to wait for fences!\n");
	}

	vk::ResultValue<uint32_t> image = device.acquireNextImageKHR(
			_pContext->swapchain, UINT64_MAX, _presentSemaphores[_frame], VK_NULL_HANDLE);

	uint32_t imageIndex = image.value;

	if (image.result == vk::Result::eErrorOutOfDateKHR) {
		_pContext->recreateSwapchain(_width, _height);
		updateInputAttachment(device, _pContext->colorView, _inputAttachmentSet);
	} else if (image.result != vk::Result::eSuccess && image.result != vk::Result::eSuboptimalKHR) {
		printf("Failed to acquire swapchain image!");
	}

	_updateUniformBuffer(_pCamera);

	device.resetFences(_fences[_frame]);

	commandBuffer.reset();

	vk::CommandBufferBeginInfo beginInfo{};

	commandBuffer.begin(beginInfo);

	std::array<vk::ClearValue, 3> clearValues;
	clearValues[0] = vk::ClearValue();
	clearValues[1].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
	clearValues[2].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

	vk::Extent2D extent = _pContext->swapchainExtent;

	vk::Viewport viewport = vk::Viewport()
									.setX(0.0f)
									.setY(0.0f)
									.setWidth(extent.width)
									.setHeight(extent.height)
									.setMinDepth(0.0f)
									.setMaxDepth(1.0f);

	vk::Rect2D scissor = vk::Rect2D().setOffset({ 0, 0 }).setExtent(extent);

	vk::RenderPassBeginInfo renderPassInfo =
			vk::RenderPassBeginInfo()
					.setRenderPass(_pContext->renderPass)
					.setFramebuffer(_pContext->swapchainImages[imageIndex].framebuffer)
					.setRenderArea(vk::Rect2D{ { 0, 0 }, extent })
					.setClearValues(clearValues);

	commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

	commandBuffer.setViewport(0, viewport);
	commandBuffer.setScissor(0, scissor);

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _materialPipeline);
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _materialLayout, 0,
			{ _uniformSets[_frame], _lightSet }, {});

	glm::mat4 view = _pCamera->viewMatrix();

	// draw geometry
	for (const Mesh &mesh : _meshes) {
		glm::mat4 modelView = mesh.transform * view;

		MeshPushConstants constants{};
		constants.model = mesh.transform;
		constants.modelViewNormal = glm::transpose(glm::inverse(modelView));

		commandBuffer.pushConstants(_materialLayout, vk::ShaderStageFlagBits::eVertex, 0,
				sizeof(MeshPushConstants), &constants);

		vk::DeviceSize offset = 0;
		commandBuffer.bindVertexBuffers(0, 1, &mesh.vertexBuffer.buffer, &offset);
		commandBuffer.bindIndexBuffer(mesh.indexBuffer.buffer, 0, vk::IndexType::eUint32);
		commandBuffer.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
	}

	commandBuffer.nextSubpass(vk::SubpassContents::eInline);

	// tonemapping

	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, _tonemapPipeline);

	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, _tonemapLayout, 0, 1,
			&_inputAttachmentSet, 0, nullptr);

	commandBuffer.draw(3, 1, 0, 0);

	// ImGui

	commandBuffer.endRenderPass();
	commandBuffer.end();

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submitInfo = vk::SubmitInfo()
										.setWaitSemaphores(_presentSemaphores[_frame])
										.setWaitDstStageMask(waitStage)
										.setCommandBuffers(commandBuffer)
										.setSignalSemaphores(_renderSemaphores[_frame]);

	_pContext->graphicsQueue.submit(submitInfo, _fences[_frame]);

	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR()
											 .setWaitSemaphores(_renderSemaphores[_frame])
											 .setSwapchains(_pContext->swapchain)
											 .setImageIndices(imageIndex);

	vk::Result err = _pContext->presentQueue.presentKHR(presentInfo);

	if (err == vk::Result::eErrorOutOfDateKHR || err == vk::Result::eSuboptimalKHR || _resized) {
		_pContext->recreateSwapchain(_width, _height);
		updateInputAttachment(device, _pContext->colorView, _inputAttachmentSet);

		_resized = false;
	} else if (err != vk::Result::eSuccess) {
		printf("Failed to present swapchain image!\n");
	}

	_frame = (_frame + 1) % FRAMES_IN_FLIGHT;
}

void RenderingDevice::createWindow(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
	_pContext->initialize(surface, width, height);

	// allocator

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
	allocatorCreateInfo.instance = _pContext->instance;
	allocatorCreateInfo.physicalDevice = _pContext->physicalDevice;
	allocatorCreateInfo.device = _pContext->device;

	{
		VkResult err = vmaCreateAllocator(&allocatorCreateInfo, &_allocator);

		if (err != VK_SUCCESS)
			printf("Failed to create allocator!\n");
	}

	// commands

	vk::Device device = _pContext->device;

	vk::CommandBufferAllocateInfo allocInfo = { _pContext->commandPool,
		vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(FRAMES_IN_FLIGHT) };

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		vk::Result err = device.allocateCommandBuffers(&allocInfo, _commandBuffers);

		if (err != vk::Result::eSuccess)
			printf("Failed to allocate command buffers!\n");
	}

	// sync

	vk::SemaphoreCreateInfo semaphoreInfo = {};
	vk::FenceCreateInfo fenceInfo = { vk::FenceCreateFlagBits::eSignaled };

	for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		_presentSemaphores[i] = device.createSemaphore(semaphoreInfo);
		_renderSemaphores[i] = device.createSemaphore(semaphoreInfo);
		_fences[i] = device.createFence(fenceInfo);
	}

	// descriptor pool

	std::array<vk::DescriptorPoolSize, 3> poolSizes;
	poolSizes[0] = { vk::DescriptorType::eUniformBuffer, FRAMES_IN_FLIGHT };
	poolSizes[1] = { vk::DescriptorType::eInputAttachment, 1 };
	poolSizes[2] = { vk::DescriptorType::eStorageBuffer, 1 };

	vk::DescriptorPoolCreateInfo createInfo =
			vk::DescriptorPoolCreateInfo()
					.setMaxSets(static_cast<uint32_t>(FRAMES_IN_FLIGHT) + 2)
					.setPoolSizes(poolSizes);

	_descriptorPool = device.createDescriptorPool(createInfo);

	// uniform

	{
		vk::DescriptorSetLayoutBinding binding =
				vk::DescriptorSetLayoutBinding()
						.setBinding(0)
						.setDescriptorType(vk::DescriptorType::eUniformBuffer)
						.setDescriptorCount(1)
						.setStageFlags(vk::ShaderStageFlagBits::eVertex |
								vk::ShaderStageFlagBits::eFragment);

		vk::DescriptorSetLayoutCreateInfo createInfo =
				vk::DescriptorSetLayoutCreateInfo().setBindings(binding);

		vk::Result err = device.createDescriptorSetLayout(&createInfo, nullptr, &_uniformLayout);

		if (err != vk::Result::eSuccess) {
			printf("Failed to create uniform layout!\n");
		}

		std::vector<vk::DescriptorSetLayout> layouts(FRAMES_IN_FLIGHT, _uniformLayout);

		vk::DescriptorSetAllocateInfo allocInfo =
				vk::DescriptorSetAllocateInfo()
						.setDescriptorPool(_descriptorPool)
						.setDescriptorSetCount(static_cast<uint32_t>(FRAMES_IN_FLIGHT))
						.setSetLayouts(layouts);

		std::array<vk::DescriptorSet, FRAMES_IN_FLIGHT> uniformSets{};

		err = device.allocateDescriptorSets(&allocInfo, uniformSets.data());

		if (err != vk::Result::eSuccess) {
			printf("Failed to allocate uniform set!\n");
		}

		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			_uniformBuffers[i] = createBuffer(_allocator, sizeof(UniformBufferObject),
					vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
					_uniformAllocInfos[i]);

			_uniformSets[i] = uniformSets[i];

			vk::DescriptorBufferInfo bufferInfo = vk::DescriptorBufferInfo()
														  .setBuffer(_uniformBuffers[i].buffer)
														  .setOffset(0)
														  .setRange(sizeof(UniformBufferObject));

			vk::WriteDescriptorSet writeInfo =
					vk::WriteDescriptorSet()
							.setDstSet(_uniformSets[i])
							.setDstBinding(0)
							.setDstArrayElement(0)
							.setDescriptorType(vk::DescriptorType::eUniformBuffer)
							.setDescriptorCount(1)
							.setBufferInfo(bufferInfo);

			device.updateDescriptorSets(writeInfo, nullptr);
		}
	}

	// input attachment

	{
		vk::DescriptorSetLayoutBinding binding =
				vk::DescriptorSetLayoutBinding()
						.setBinding(0)
						.setDescriptorType(vk::DescriptorType::eInputAttachment)
						.setDescriptorCount(1)
						.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::DescriptorSetLayoutCreateInfo createInfo =
				vk::DescriptorSetLayoutCreateInfo().setBindings(binding);

		vk::Result err =
				device.createDescriptorSetLayout(&createInfo, nullptr, &_inputAttachmentLayout);

		if (err != vk::Result::eSuccess) {
			printf("Failed to create image attachment layout!\n");
		}

		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
														  .setDescriptorPool(_descriptorPool)
														  .setDescriptorSetCount(1)
														  .setSetLayouts(_inputAttachmentLayout);

		err = device.allocateDescriptorSets(&allocInfo, &_inputAttachmentSet);

		if (err != vk::Result::eSuccess) {
			printf("Failed to allocate input attachment set!\n");
		}

		updateInputAttachment(device, _pContext->colorView, _inputAttachmentSet);
	}

	// light

	{
		vk::DescriptorSetLayoutBinding binding =
				vk::DescriptorSetLayoutBinding()
						.setBinding(0)
						.setDescriptorType(vk::DescriptorType::eStorageBuffer)
						.setDescriptorCount(1)
						.setStageFlags(vk::ShaderStageFlagBits::eFragment);

		vk::DescriptorSetLayoutCreateInfo createInfo =
				vk::DescriptorSetLayoutCreateInfo().setBindings(binding);

		vk::Result err = device.createDescriptorSetLayout(&createInfo, nullptr, &_lightLayout);

		if (err != vk::Result::eSuccess) {
			printf("Failed to create light layout!\n");
		}

		vk::DescriptorSetAllocateInfo allocInfo = vk::DescriptorSetAllocateInfo()
														  .setDescriptorPool(_descriptorPool)
														  .setDescriptorSetCount(1)
														  .setSetLayouts(_lightLayout);

		err = device.allocateDescriptorSets(&allocInfo, &_lightSet);

		if (err != vk::Result::eSuccess) {
			printf("Failed to allocate light set!\n");
		}

		vk::DeviceSize lightBufferSize = sizeof(LightData) * MAX_LIGHT_COUNT;

		VmaAllocationInfo _allocInfo;
		_lightBuffer = createBuffer(_allocator, lightBufferSize,
				vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
				_allocInfo);

		vk::DescriptorBufferInfo bufferInfo = vk::DescriptorBufferInfo()
													  .setBuffer(_lightBuffer.buffer)
													  .setOffset(0)
													  .setRange(lightBufferSize);

		vk::WriteDescriptorSet writeInfo =
				vk::WriteDescriptorSet()
						.setDstSet(_lightSet)
						.setDstBinding(0)
						.setDstArrayElement(0)
						.setDescriptorType(vk::DescriptorType::eStorageBuffer)
						.setDescriptorCount(1)
						.setBufferInfo(bufferInfo);

		device.updateDescriptorSets(writeInfo, nullptr);
	}

	// material

	{
		MaterialShader shader;

		vk::ShaderModule vertexStage =
				createShaderModule(device, shader.vertexCode, sizeof(shader.vertexCode));

		vk::ShaderModule fragmentStage =
				createShaderModule(device, shader.fragmentCode, sizeof(shader.fragmentCode));

		std::array<vk::DescriptorSetLayout, 2> layouts = { _uniformLayout, _lightLayout };

		vk::PushConstantRange pushConstant = vk::PushConstantRange(
				vk::ShaderStageFlagBits::eVertex, 0, sizeof(MeshPushConstants));

		vk::PipelineLayoutCreateInfo createInfo =
				vk::PipelineLayoutCreateInfo().setSetLayouts(layouts).setPushConstantRanges(
						pushConstant);

		vk::VertexInputBindingDescription binding = Vertex::getBindingDescription();
		std::array<vk::VertexInputAttributeDescription, 4> attribute =
				Vertex::getAttributeDescriptions();

		vk::PipelineVertexInputStateCreateInfo vertexInput =
				vk::PipelineVertexInputStateCreateInfo()
						.setVertexBindingDescriptions(binding)
						.setVertexAttributeDescriptions(attribute);

		_materialLayout = device.createPipelineLayout(createInfo);
		_materialPipeline = createPipeline(device, vertexStage, fragmentStage, _materialLayout,
				_pContext->renderPass, 0, vertexInput);

		device.destroyShaderModule(vertexStage);
		device.destroyShaderModule(fragmentStage);
	}

	// tonemapping

	{
		TonemapShader shader;

		vk::ShaderModule vertexStage =
				createShaderModule(device, shader.vertexCode, sizeof(shader.vertexCode));

		vk::ShaderModule fragmentStage =
				createShaderModule(device, shader.fragmentCode, sizeof(shader.fragmentCode));

		vk::PipelineLayoutCreateInfo createInfo =
				vk::PipelineLayoutCreateInfo().setSetLayouts(_inputAttachmentLayout);

		_tonemapLayout = device.createPipelineLayout(createInfo);
		_tonemapPipeline = createPipeline(
				device, vertexStage, fragmentStage, _tonemapLayout, _pContext->renderPass, 1, {});

		device.destroyShaderModule(vertexStage);
		device.destroyShaderModule(fragmentStage);
	}
}

void RenderingDevice::resizeWindow(uint32_t width, uint32_t height) {
	if (_width == width && _height == height) {
		return;
	}

	_width = width;
	_height = height;
	_resized = true;
}

vk::Instance RenderingDevice::getInstance() { return _pContext->instance; }

RenderingDevice::RenderingDevice(std::vector<const char *> extensions, bool useValidation) {
	_pContext = new VulkanContext(extensions, useValidation);
}
