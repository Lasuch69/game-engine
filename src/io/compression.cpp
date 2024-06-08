#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <cvtt/ConvectionKernels.h>

#include <io/image.h>
#include <rendering/rendering_device.h>

#include "compression.h"

const uint32_t BLOCK_SIZE = 4;
const uint32_t CHUNK_BYTE_SIZE = cvtt::NumParallelBlocks * 16;

uint32_t _blockSizeCeil(uint32_t value) {
	return ((value + (BLOCK_SIZE - 1)) / BLOCK_SIZE) * BLOCK_SIZE;
}

uint32_t _clamp(uint32_t value, uint32_t min, uint32_t max) {
	if (value > max)
		return max;

	if (value < min)
		return min;

	return value;
}

// Offset of pixel(x, y) in pBuffer
uint16_t *_pixel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t *pBuffer) {
	x = _clamp(x, 0, w - 1);
	y = _clamp(y, 0, h - 1);

	uint32_t offset = ((y * w) + x) * 4;
	return &pBuffer[offset];
}

cvtt::PixelBlockF16 _block(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t *pBuffer) {
	cvtt::PixelBlockF16 block;

	uint32_t i = 0;
	for (uint32_t py = 0; py < 4; py++) {
		for (uint32_t px = 0; px < 4; px++) {
			uint16_t *pBytes = _pixel(px + x, py + y, w, h, pBuffer);

			block.m_pixels[i][0] = pBytes[0];
			block.m_pixels[i][1] = pBytes[1];
			block.m_pixels[i][2] = pBytes[2];
			block.m_pixels[i][3] = pBytes[3];

			i++;
		}
	}

	return block;
}

bool Compression::imageCompress(Image *pImage) {
	Image::Format format = pImage->getFormat();

	if (format != Image::Format::RGBAF16)
		return false;

	uint32_t width = pImage->getWidth();
	uint32_t height = pImage->getHeight();

	std::vector<uint8_t> data = pImage->getData();
	uint16_t *pData = reinterpret_cast<uint16_t *>(data.data());

	uint32_t pixelCount = width * height * 4;

	bool isSigned = false;
	for (uint32_t i = 0; i < pixelCount; i++) {
		int sign = (pData[i] >> 15) & 0x1;
		isSigned = sign == 1;

		if (isSigned)
			break;
	}

	uint32_t widthCeiled = _blockSizeCeil(width);
	uint32_t heightCeiled = _blockSizeCeil(height);

	uint8_t *pBuffer = (uint8_t *)malloc(widthCeiled * heightCeiled);
	uint32_t offset = 0;

	cvtt::Options options;
	options.flags = cvtt::Flags::Ultra;

	for (uint32_t startY = 0; startY < height; startY += BLOCK_SIZE) {
		for (uint32_t startX = 0; startX < width; startX += BLOCK_SIZE * cvtt::NumParallelBlocks) {
			cvtt::PixelBlockF16 blocks[cvtt::NumParallelBlocks];

			for (int i = 0; i < 8; i++) {
				uint32_t x = startX + BLOCK_SIZE * i;
				blocks[i] = _block(x, startY, width, height, pData);
			}

			uint8_t chunk[CHUNK_BYTE_SIZE];

			if (isSigned) {
				cvtt::Kernels::EncodeBC6HS(chunk, blocks, options);
			} else {
				cvtt::Kernels::EncodeBC6HU(chunk, blocks, options);
			}

			memcpy(&pBuffer[offset], chunk, CHUNK_BYTE_SIZE);
			offset += CHUNK_BYTE_SIZE;
		}
	}

	RD &rd = RD::getSingleton();

	vk::Format _format = vk::Format::eBc6HUfloatBlock;

	if (isSigned)
		_format = vk::Format::eBc6HSfloatBlock;

	AllocatedImage image =
			rd.imageCreate(width, height, _format, 1, vk::ImageUsageFlagBits::eTransferDst);

	rd.imageLayoutTransition(image.image, _format, 1, 1, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal);

	uint32_t size = widthCeiled * heightCeiled;
	rd.imageSend(image.image, width, height, pBuffer, size, vk::ImageLayout::eTransferDstOptimal);

	free(pBuffer);
	return true;
}
