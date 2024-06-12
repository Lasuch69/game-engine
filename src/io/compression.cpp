#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <cvtt/ConvectionKernels.h>

#include <math/float16.h>
#include <math/util.h>

#include "image.h"

#include "compression.h"

const uint32_t BLOCK_SIZE = 4;
const size_t BLOCK_BYTE_SIZE = 16;
const uint32_t CHUNK_BYTE_SIZE = cvtt::NumParallelBlocks * 16;
const uint32_t CHANNEL_COUNT = 4;

typedef struct {
	half channels[4];
} Color;

static Color getPixel(
		uint32_t x, uint32_t y, uint32_t width, uint32_t height, const half *pBuffer) {
	x = min<uint32_t>(x, width - 1);
	y = min<uint32_t>(y, height - 1);

	uint32_t offset = ((y * width) + x) * CHANNEL_COUNT;

	Color color;
	color.channels[0] = pBuffer[offset + 0];
	color.channels[1] = pBuffer[offset + 1];
	color.channels[2] = pBuffer[offset + 2];
	color.channels[3] = pBuffer[offset + 3];

	return color;
}

static cvtt::PixelBlockF16 getBlock(
		uint32_t x, uint32_t y, uint32_t width, uint32_t height, const half *pBuffer) {
	cvtt::PixelBlockF16 block;

	uint32_t i = 0;
	for (uint32_t py = 0; py < BLOCK_SIZE; py++) {
		for (uint32_t px = 0; px < BLOCK_SIZE; px++) {
			Color color = getPixel(px + x, py + y, width, height, pBuffer);

			block.m_pixels[i][0] = color.channels[0];
			block.m_pixels[i][1] = color.channels[1];
			block.m_pixels[i][2] = color.channels[2];
			block.m_pixels[i][3] = color.channels[3];

			i++;
		}
	}

	return block;
}

static uint32_t blockCeil(uint32_t value) {
	return ((value + (BLOCK_SIZE - 1)) / BLOCK_SIZE) * BLOCK_SIZE;
}

static std::vector<uint8_t> compressStep(uint32_t width, uint32_t height, const half *pData) {
	cvtt::Options options;
	options.flags = cvtt::Flags::Ultra;

	size_t offset = 0;
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

			size_t numBytes = min<size_t>(blockCount * BLOCK_BYTE_SIZE, outputSize - offset);
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

	if (!pImage->generateMipmaps())
		return false;

	uint32_t width = pImage->getWidth();
	uint32_t height = pImage->getHeight();
	size_t dataSize = width * height * CHANNEL_COUNT;

	std::vector<uint8_t> data = pImage->getData();
	half *pData = reinterpret_cast<half *>(data.data());

	std::vector<uint8_t> output = compressStep(width, height, pData);

	uint32_t mipLevels = pImage->getMipLevels();

	uint32_t lastWidth = width;
	uint32_t lastHeight = height;
	size_t offset = dataSize;

	for (uint32_t i = 1; i < mipLevels; i++) {
		uint32_t mipmapWidth = max<uint32_t>(lastWidth >> 1, 1);
		uint32_t mipmapHeight = max<uint32_t>(lastHeight >> 1, 1);

		std::vector<uint8_t> output = compressStep(mipmapWidth, mipmapHeight, &pData[offset]);

		lastWidth = mipmapWidth;
		lastHeight = mipmapHeight;
		offset += mipmapWidth * mipmapHeight * CHANNEL_COUNT;
	}

	return true;
}
