#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <cvtt/ConvectionKernels.h>

#include <math/float16.h>
#include <rendering/rendering_device.h>
#include <rendering/types/allocated.h>

#include "image.h"

#include "compression.h"

const uint32_t BLOCK_SIZE = 4;
const uint32_t BLOCK_BYTE_SIZE = 16;

const uint32_t CHUNK_BYTE_SIZE = cvtt::NumParallelBlocks * 16;

const uint32_t CHANNEL_COUNT = 4;

typedef struct {
	half channels[4];
} ColorF16;

static uint32_t max(uint32_t a, uint32_t b) {
	return (a > b) ? a : b;
}

static uint32_t min(uint32_t a, uint32_t b) {
	return (a < b) ? a : b;
}

// Offset of pixel(x, y) in pBuffer
static ColorF16 getPixel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const half *pBuffer) {
	x = min(x, w - 1);
	y = min(y, h - 1);

	uint32_t offset = ((y * w) + x) * CHANNEL_COUNT;

	ColorF16 color;
	color.channels[0] = pBuffer[offset + 0];
	color.channels[1] = pBuffer[offset + 1];
	color.channels[2] = pBuffer[offset + 2];
	color.channels[3] = pBuffer[offset + 3];

	return color;
}

static cvtt::PixelBlockF16 getBlock(
		uint32_t x, uint32_t y, uint32_t w, uint32_t h, const half *pBuffer) {
	cvtt::PixelBlockF16 block;

	uint32_t i = 0;
	for (uint32_t py = 0; py < BLOCK_SIZE; py++) {
		for (uint32_t px = 0; px < BLOCK_SIZE; px++) {
			ColorF16 color = getPixel(px + x, py + y, w, h, pBuffer);

			block.m_pixels[i][0] = color.channels[0];
			block.m_pixels[i][1] = color.channels[1];
			block.m_pixels[i][2] = color.channels[2];
			block.m_pixels[i][3] = color.channels[3];

			i++;
		}
	}

	return block;
}

typedef struct {
	uint32_t width, height;
	size_t bufferSize;
	half *pBuffer;
} Level;

static bool generateMipmaps(uint32_t width, uint32_t height, std::vector<half> &data) {
	uint32_t lastWidth = width;
	uint32_t lastHeight = height;
	std::vector<half> lastData = data;

	while (lastWidth != 1 || lastHeight != 1) {
		uint32_t mipmapWidth = max(lastWidth >> 1, 1);
		uint32_t mipmapHeight = max(lastHeight >> 1, 1);
		std::vector<half> mipmapData(mipmapWidth * mipmapHeight * CHANNEL_COUNT);

		uint32_t idx = 0;
		for (uint32_t y = 0; y < lastHeight; y += 2) {
			for (uint32_t x = 0; x < lastWidth; x += 2) {
				ColorF16 p00 = getPixel(x, y, lastWidth, lastHeight, lastData.data());
				ColorF16 p10 = getPixel(x + 1, y, lastWidth, lastHeight, lastData.data());
				ColorF16 p01 = getPixel(x, y + 1, lastWidth, lastHeight, lastData.data());
				ColorF16 p11 = getPixel(x + 1, y + 1, lastWidth, lastHeight, lastData.data());

				for (int i = 0; i < CHANNEL_COUNT; i++) {
					half h0 = halfAverageApprox(p00.channels[i], p10.channels[i]);
					half h1 = halfAverageApprox(p01.channels[i], p11.channels[i]);
					mipmapData[idx] = halfAverageApprox(h0, h1);
					idx++;
				}
			}
		}

		for (uint32_t i = 0; i < mipmapData.size(); i++)
			data.push_back(mipmapData[i]);

		lastWidth = mipmapWidth;
		lastHeight = mipmapHeight;
		lastData = mipmapData;
	}

	return true;
}

static uint32_t blockCeil(uint32_t value) {
	return ((value + (BLOCK_SIZE - 1)) / BLOCK_SIZE) * BLOCK_SIZE;
}

std::vector<uint8_t> Compression::_compressStep(uint32_t width, uint32_t height, uint16_t *pData) {
	cvtt::Options options;
	options.flags = cvtt::Flags::Ultra;

	uint32_t offset = 0;
	size_t outputSize = blockCeil(width) * blockCeil(height);
	std::vector<uint8_t> outputBytes(outputSize);

	const uint32_t ROW_SIZE_X = BLOCK_SIZE * cvtt::NumParallelBlocks;
	const uint32_t ROW_SIZE_Y = BLOCK_SIZE;

	for (uint32_t offsetY = 0; offsetY < height; offsetY += ROW_SIZE_Y) {
		for (uint32_t offsetX = 0; offsetX < width; offsetX += ROW_SIZE_X) {
			cvtt::PixelBlockF16 inputBlocks[cvtt::NumParallelBlocks];

			size_t blockCount = 0;
			for (int i = 0; i < cvtt::NumParallelBlocks; i++) {
				uint32_t _offsetX = offsetX + BLOCK_SIZE * i;

				if (_offsetX >= width)
					break;

				inputBlocks[i] = getBlock(_offsetX, offsetY, width, height, pData);
				blockCount++;
			}

			uint8_t outputBlocks[CHUNK_BYTE_SIZE];
			cvtt::Kernels::EncodeBC6HU(outputBlocks, inputBlocks, options);

			size_t numBytes = min(blockCount * BLOCK_BYTE_SIZE, outputSize - offset);
			memcpy(&outputBytes[offset], outputBlocks, numBytes);
			offset += numBytes;
		}
	}

	return outputBytes;
}

bool Compression::imageCompress(Image *pImage) {
	Image::Format format = pImage->getFormat();

	if (format != Image::Format::RGBAF16)
		return false;

	uint32_t width = pImage->getWidth();
	uint32_t height = pImage->getHeight();

	size_t dataSize = width * height * CHANNEL_COUNT;

	std::vector<half> data(dataSize);
	memcpy(data.data(), pImage->getData().data(), dataSize * sizeof(half));

	std::vector<uint8_t> output = _compressStep(width, height, data.data());

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	vk::Format _format = vk::Format::eBc6HUfloatBlock;

	AllocatedImage image = RD::getSingleton().imageCreate(
			width, height, _format, mipLevels, vk::ImageUsageFlagBits::eTransferDst);

	RD::getSingleton().imageLayoutTransition(image.image, _format, mipLevels, 1,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	RD::getSingleton().imageSend(image.image, width, height, output.data(), output.size(),
			vk::ImageLayout::eTransferDstOptimal);

	generateMipmaps(width, height, data);

	uint32_t lastWidth = width;
	uint32_t lastHeight = height;
	size_t offset = dataSize;

	for (uint32_t i = 1; i < mipLevels; i++) {
		uint32_t mipmapWidth = max(lastWidth >> 1, 1);
		uint32_t mipmapHeight = max(lastHeight >> 1, 1);

		std::vector<uint8_t> output = _compressStep(mipmapWidth, mipmapHeight, &data[offset]);

		RD::getSingleton().imageSend(image.image, mipmapWidth, mipmapHeight, output.data(),
				output.size(), vk::ImageLayout::eTransferDstOptimal, i);

		lastWidth = mipmapWidth;
		lastHeight = mipmapHeight;
		offset += mipmapWidth * mipmapHeight * CHANNEL_COUNT;
	}

	return true;
}
