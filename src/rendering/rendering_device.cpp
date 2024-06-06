#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <SDL3/SDL_log.h>

#include "rendering_device.h"
#include "types/allocated.h"

vk::CommandBuffer RD::beginSingleTimeCommands() {
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
	allocInfo.setCommandPool(_pContext->getCommandPool());
	allocInfo.setCommandBufferCount(1);

	vk::CommandBuffer commandBuffer = _pContext->getDevice().allocateCommandBuffers(allocInfo)[0];

	vk::CommandBufferBeginInfo beginInfo = { vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
	commandBuffer.begin(beginInfo);

	return commandBuffer;
}

void RD::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
	commandBuffer.end();

	vk::SubmitInfo submitInfo;
	submitInfo.setCommandBufferCount(1);
	submitInfo.setCommandBuffers(commandBuffer);

	_pContext->getGraphicsQueue().submit(submitInfo, VK_NULL_HANDLE);
	_pContext->getGraphicsQueue().waitIdle();

	_pContext->getDevice().freeCommandBuffers(_pContext->getCommandPool(), commandBuffer);
}

AllocatedBuffer RD::bufferCreate(
		vk::BufferUsageFlags usage, vk::DeviceSize size, VmaAllocationInfo *pAllocInfo) {
	return AllocatedBuffer::create(_allocator, usage, size, pAllocInfo);
}

void RD::bufferCopy(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::BufferCopy bufferCopy;
	bufferCopy.setSrcOffset(0);
	bufferCopy.setDstOffset(0);
	bufferCopy.setSize(size);

	commandBuffer.copyBuffer(srcBuffer, dstBuffer, bufferCopy);

	endSingleTimeCommands(commandBuffer);
}

void RD::bufferCopyToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height,
		vk::ImageLayout layout) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::ImageSubresourceLayers imageSubresource;
	imageSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
	imageSubresource.setMipLevel(0);
	imageSubresource.setBaseArrayLayer(0);
	imageSubresource.setLayerCount(1);

	vk::BufferImageCopy region;
	region.setBufferOffset(0);
	region.setBufferRowLength(0);
	region.setBufferImageHeight(0);
	region.setImageSubresource(imageSubresource);
	region.setImageOffset(vk::Offset3D{ 0, 0, 0 });
	region.setImageExtent(vk::Extent3D{ width, height, 1 });

	commandBuffer.copyBufferToImage(buffer, image, layout, region);

	endSingleTimeCommands(commandBuffer);
}

void RD::bufferSend(vk::Buffer dstBuffer, void *pData, size_t size) {
	vk::BufferUsageFlagBits usage = vk::BufferUsageFlagBits::eTransferSrc;

	VmaAllocationInfo stagingAllocInfo;
	AllocatedBuffer stagingBuffer = bufferCreate(usage, size, &stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, pData, size);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	bufferCopy(stagingBuffer.buffer, dstBuffer, size);

	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

void RD::bufferDestroy(AllocatedBuffer buffer) {
	vmaDestroyBuffer(_allocator, buffer.buffer, buffer.allocation);
}

AllocatedImage RD::imageCreate(uint32_t width, uint32_t height, vk::Format format,
		uint32_t mipLevels, vk::ImageUsageFlags usage) {
	return AllocatedImage::create(_allocator, width, height, mipLevels, 1, format, usage);
}

AllocatedImage RD::imageCubeCreate(
		uint32_t size, vk::Format format, uint32_t mipLevels, vk::ImageUsageFlags usage) {
	uint32_t arrayLayers = 6;
	return AllocatedImage::create(_allocator, size, size, mipLevels, arrayLayers, format, usage,
			vk::ImageCreateFlagBits::eCubeCompatible);
}

void RD::imageGenerateMipmaps(vk::Image image, int32_t width, int32_t height, vk::Format format,
		uint32_t mipLevels, uint32_t arrayLayers) {
	vk::FormatProperties properties = _pContext->getPhysicalDevice().getFormatProperties(format);

	bool isBlittingSupported = (bool)(properties.optimalTilingFeatures &
									  vk::FormatFeatureFlagBits::eSampledImageFilterLinear);

	assert(isBlittingSupported);

	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::ImageSubresourceRange subresourceRange;
	subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
	subresourceRange.setLevelCount(1);
	subresourceRange.setBaseArrayLayer(0);
	subresourceRange.setLayerCount(arrayLayers);

	vk::ImageMemoryBarrier barrier;
	barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setImage(image);

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < mipLevels; i++) {
		subresourceRange.setBaseMipLevel(i - 1);
		barrier.setSubresourceRange(subresourceRange);
		barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
		barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer, {}, nullptr, nullptr, barrier);

		std::array<vk::Offset3D, 2> srcOffsets = {
			vk::Offset3D(0, 0, 0),
			vk::Offset3D(mipWidth, mipHeight, 1),
		};

		std::array<vk::Offset3D, 2> dstOffsets = {
			vk::Offset3D(0, 0, 0),
			vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1),
		};

		vk::ImageSubresourceLayers srcSubresource;
		srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
		srcSubresource.setMipLevel(i - 1);
		srcSubresource.setBaseArrayLayer(0);
		srcSubresource.setLayerCount(arrayLayers);

		vk::ImageSubresourceLayers dstSubresource;
		dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor);
		dstSubresource.setMipLevel(i);
		dstSubresource.setBaseArrayLayer(0);
		dstSubresource.setLayerCount(arrayLayers);

		vk::ImageBlit blit;
		blit.setSrcOffsets(srcOffsets);
		blit.setDstOffsets(dstOffsets);
		blit.setSrcSubresource(srcSubresource);
		blit.setDstSubresource(dstSubresource);

		commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
				vk::ImageLayout::eTransferDstOptimal, blit, vk::Filter::eLinear);

		barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
		barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
		barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	subresourceRange.setBaseMipLevel(mipLevels - 1);
	barrier.setSubresourceRange(subresourceRange);
	barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
	barrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
	barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, barrier);

	endSingleTimeCommands(commandBuffer);
}

void RD::imageLayoutTransition(vk::Image image, vk::Format format, uint32_t mipLevels,
		uint32_t arrayLayers, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
	vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

	vk::ImageSubresourceRange subresourceRange;
	subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
	subresourceRange.setBaseMipLevel(0);
	subresourceRange.setLevelCount(mipLevels);
	subresourceRange.setBaseArrayLayer(0);
	subresourceRange.setLayerCount(arrayLayers);

	vk::ImageMemoryBarrier barrier;
	barrier.setOldLayout(oldLayout);
	barrier.setNewLayout(newLayout);
	barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
	barrier.setImage(image);
	barrier.setSubresourceRange(subresourceRange);

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (oldLayout == vk::ImageLayout::eUndefined &&
			newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
		barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
			   newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	} else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eGeneral) {
		barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
		barrier.setDstAccessMask(vk::AccessFlagBits::eNone);

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eComputeShader;
	} else if (oldLayout == vk::ImageLayout::eGeneral &&
			   newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

		sourceStage = vk::PipelineStageFlagBits::eComputeShader;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	} else if (oldLayout == vk::ImageLayout::eGeneral &&
			   newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

		sourceStage = vk::PipelineStageFlagBits::eComputeShader;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	} else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal &&
			   newLayout == vk::ImageLayout::eTransferSrcOptimal) {
		barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
		barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

		sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	} else {
		throw std::invalid_argument("Unsupported layout transition!");
	}

	commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);

	endSingleTimeCommands(commandBuffer);
}

void RD::imageSend(vk::Image image, uint32_t width, uint32_t height, uint8_t *pData, size_t size,
		vk::ImageLayout layout) {
	VmaAllocationInfo stagingAllocInfo;
	vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferSrc;

	AllocatedBuffer stagingBuffer = bufferCreate(usage, size, &stagingAllocInfo);
	memcpy(stagingAllocInfo.pMappedData, pData, size);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);

	bufferCopyToImage(stagingBuffer.buffer, image, width, height, layout);

	bufferDestroy(stagingBuffer);
}

void RD::imageDestroy(AllocatedImage image) {
	vmaDestroyImage(_allocator, image.image, image.allocation);
}

vk::ImageView RD::imageViewCreate(vk::Image image, vk::Format format, uint32_t mipLevels,
		uint32_t arrayLayers, vk::ImageViewType viewType) {
	vk::ImageSubresourceRange subresourceRange;
	subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
	subresourceRange.setBaseMipLevel(0);
	subresourceRange.setLevelCount(mipLevels);
	subresourceRange.setBaseArrayLayer(0);
	subresourceRange.setLayerCount(arrayLayers);

	vk::ImageViewCreateInfo createInfo;
	createInfo.setImage(image);
	createInfo.setViewType(viewType);
	createInfo.setFormat(format);
	createInfo.setSubresourceRange(subresourceRange);

	return _pContext->getDevice().createImageView(createInfo);
}

void RD::imageViewDestroy(vk::ImageView imageView) {
	_pContext->getDevice().destroyImageView(imageView);
}

vk::Sampler RD::samplerCreate(vk::Filter filter, vk::SamplerAddressMode repeatMode,
		uint32_t mipLevels, float mipLodBias) {
	vk::PhysicalDeviceProperties properties = _pContext->getPhysicalDevice().getProperties();
	float maxAnisotropy = properties.limits.maxSamplerAnisotropy;

	vk::SamplerCreateInfo createInfo;
	createInfo.setMagFilter(filter);
	createInfo.setMinFilter(filter);
	createInfo.setAddressModeU(repeatMode);
	createInfo.setAddressModeV(repeatMode);
	createInfo.setAddressModeW(repeatMode);
	createInfo.setAnisotropyEnable(true);
	createInfo.setMaxAnisotropy(maxAnisotropy);
	createInfo.setBorderColor(vk::BorderColor::eIntOpaqueBlack);
	createInfo.setUnnormalizedCoordinates(false);
	createInfo.setCompareEnable(false);
	createInfo.setCompareOp(vk::CompareOp::eAlways);
	createInfo.setMipmapMode(vk::SamplerMipmapMode::eLinear);
	createInfo.setMinLod(0.0f);
	createInfo.setMaxLod(static_cast<float>(mipLevels));
	createInfo.setMipLodBias(mipLodBias);

	return _pContext->getDevice().createSampler(createInfo);
}

void RD::samplerDestroy(vk::Sampler sampler) {
	_pContext->getDevice().destroySampler(sampler);
}

vk::CommandBuffer RD::drawBegin() {
	vk::CommandBuffer commandBuffer = _commandBuffers[_frame];

	vk::Result result = _pContext->getDevice().waitForFences(_fences[_frame], VK_TRUE, UINT64_MAX);

	if (result != vk::Result::eSuccess)
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Waiting for fences failed!");

	vk::ResultValue<uint32_t> image = _pContext->getDevice().acquireNextImageKHR(
			_pContext->getSwapchain(), UINT64_MAX, _presentSemaphores[_frame], VK_NULL_HANDLE);

	_imageIndex = image.value;

	if (image.result == vk::Result::eErrorOutOfDateKHR) {
		_pContext->recreateSwapchain(_width, _height);
		_onSwapchainResized(_pContext->getColorAttachment().getImageView());
	} else if (image.result != vk::Result::eSuccess && image.result != vk::Result::eSuboptimalKHR) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Swapchain image acquire failed!");
	}

	_pContext->getDevice().resetFences(_fences[_frame]);

	commandBuffer.reset();

	vk::CommandBufferBeginInfo beginInfo{};

	commandBuffer.begin(beginInfo);

	std::array<vk::ClearValue, 3> clearValues;
	clearValues[0] = vk::ClearValue();
	clearValues[1].color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
	clearValues[2].depthStencil = vk::ClearDepthStencilValue(0.0f, 0);

	vk::Extent2D extent = _pContext->getSwapchainExtent();

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
	renderPassInfo.setRenderPass(_pContext->getRenderPass());
	renderPassInfo.setFramebuffer(_pContext->getFramebuffer(_imageIndex.value()));
	renderPassInfo.setRenderArea(renderArea);
	renderPassInfo.setClearValues(clearValues);

	commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

	commandBuffer.setViewport(0, viewport);
	commandBuffer.setScissor(0, scissor);

	return commandBuffer;
}

void RD::drawEnd(vk::CommandBuffer commandBuffer) {
	commandBuffer.endRenderPass();
	commandBuffer.end();

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submitInfo;
	submitInfo.setWaitSemaphores(_presentSemaphores[_frame]);
	submitInfo.setWaitDstStageMask(waitStage);
	submitInfo.setCommandBuffers(commandBuffer);
	submitInfo.setSignalSemaphores(_renderSemaphores[_frame]);

	_pContext->getGraphicsQueue().submit(submitInfo, _fences[_frame]);

	vk::SwapchainKHR swapchain = _pContext->getSwapchain();

	vk::PresentInfoKHR presentInfo;
	presentInfo.setWaitSemaphores(_renderSemaphores[_frame]);
	presentInfo.setSwapchains(swapchain);
	presentInfo.setImageIndices(_imageIndex.value());

	vk::Result err = _pContext->getPresentQueue().presentKHR(presentInfo);

	if (err == vk::Result::eErrorOutOfDateKHR || err == vk::Result::eSuboptimalKHR || _resized) {
		_pContext->recreateSwapchain(_width, _height);
		_onSwapchainResized(_pContext->getColorAttachment().getImageView());

		_resized = false;
	} else if (err != vk::Result::eSuccess) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Swapchain image presentation failed!");
	}

	_imageIndex.reset();
	_frame = (_frame + 1) % FRAMES_IN_FLIGHT;
}

void RD::windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height) {
	_pContext->initialize(surface, width, height);

	// allocator

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
	allocatorCreateInfo.instance = _pContext->getInstance();
	allocatorCreateInfo.physicalDevice = _pContext->getPhysicalDevice();
	allocatorCreateInfo.device = _pContext->getDevice();

	{
		VkResult err = vmaCreateAllocator(&allocatorCreateInfo, &_allocator);

		if (err != VK_SUCCESS)
			throw std::runtime_error("VmaAllocator creation failed!");
	}

	// commands

	vk::Device device = _pContext->getDevice();

	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.setCommandPool(_pContext->getCommandPool());
	allocInfo.setLevel(vk::CommandBufferLevel::ePrimary);
	allocInfo.setCommandBufferCount(static_cast<uint32_t>(FRAMES_IN_FLIGHT));

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		vk::Result err = device.allocateCommandBuffers(&allocInfo, _commandBuffers);

		if (err != vk::Result::eSuccess)
			throw std::runtime_error("Command buffers allocation failed!");
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

	std::array<vk::DescriptorPoolSize, 4> poolSizes;
	poolSizes[0] = { vk::DescriptorType::eUniformBuffer, FRAMES_IN_FLIGHT };
	poolSizes[1] = { vk::DescriptorType::eInputAttachment, 1 };
	poolSizes[2] = { vk::DescriptorType::eStorageBuffer, 1 };
	poolSizes[3] = { vk::DescriptorType::eCombinedImageSampler, 1000 };

	uint32_t maxSets = 0;

	for (const vk::DescriptorPoolSize &poolSize : poolSizes) {
		maxSets += poolSize.descriptorCount;
	}

	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.setMaxSets(maxSets);
	createInfo.setPoolSizes(poolSizes);

	_descriptorPool = device.createDescriptorPool(createInfo);
}

void RD::windowResize(uint32_t width, uint32_t height) {
	if (_width == width && _height == height) {
		return;
	}

	_width = width;
	_height = height;
	_resized = true;
}

void RD::init(bool useValidation) {
	_pContext = new VulkanContext(useValidation);
}
