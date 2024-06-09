#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <cvtt/ConvectionKernels.h>

#include <io/image.h>
#include <rendering/rendering_device.h>
#include <vector>

#include "compression.h"
#include "rendering/types/allocated.h"

const uint32_t BLOCK_SIZE = 4;
const uint32_t CHUNK_BYTE_SIZE = cvtt::NumParallelBlocks * 16;

#define FLOAT16_ONE 0x3c00

#define FLOAT16_MAX 0x7bff // 01111011 11111111 =  65504.0
#define FLOAT16_MIN 0xfbff // 11111011 11111111 = -65504.0

static uint16_t _floatToHalf(float value) {
	union {
		float value;
		uint32_t bits;
	} data;

	data.value = value;
	uint32_t bits = data.bits;

	uint32_t sign = (bits >> 31) & 0x1;
	uint32_t exp = (bits >> 23) & 0xff;
	uint32_t frac = bits & 0x7fffff;

	bool isNegative = sign == 1;

	if (exp == 0xff) {
		// NaN
		if (frac > 0)
			return 0;

		// infinity
		return isNegative ? FLOAT16_MIN : FLOAT16_MAX;
	}

	exp = exp - 127 + 15;
	if (exp >= 0x1f)
		return isNegative ? FLOAT16_MIN : FLOAT16_MAX;

	frac = frac >> 13;
	return (sign << 15) + (exp << 10) + frac;
}

static float _halfToFloat(uint16_t value) {
	uint32_t sign = (value >> 15) & 0x1;
	uint32_t exp = (value >> 10) & 0x1f;
	uint32_t frac = value & 0x3ff;

	exp = exp + (127 - 15);

	union {
		float value;
		uint32_t bits;
	} data;

	data.bits = (sign << 31) + (exp << 23) + (frac << 13);
	return data.value;
}

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
const uint16_t *_pixel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *pBuffer) {
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
			const uint16_t *pBytes = _pixel(px + x, py + y, w, h, pBuffer);

			block.m_pixels[i][0] = pBytes[0];
			block.m_pixels[i][1] = pBytes[1];
			block.m_pixels[i][2] = pBytes[2];
			block.m_pixels[i][3] = pBytes[3];

			i++;
		}
	}

	return block;
}

void _generateMipmaps(uint32_t width, uint32_t height, const std::vector<uint8_t> &data) {
	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	uint32_t lastWidth = width;
	uint32_t lastHeight = height;

	std::vector<uint16_t> lastData(data.size() / sizeof(uint16_t));
	memcpy(lastData.data(), data.data(), data.size());

	RenderingDevice &rd = RenderingDevice::getSingleton();

	vk::Format format = vk::Format::eR16G16B16A16Sfloat;
	AllocatedImage image =
			rd.imageCreate(width, height, format, mipLevels, vk::ImageUsageFlagBits::eTransferDst);

	rd.imageLayoutTransition(image.image, format, mipLevels, 1, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal);

	rd.imageSend(image.image, width, height, (uint8_t *)data.data(), data.size(),
			vk::ImageLayout::eTransferDstOptimal);

	for (uint32_t level = 1; level < mipLevels; level++) {
		std::vector<uint16_t> buffer;
		for (uint32_t y = 0; y < lastHeight; y += 2) {
			for (uint32_t x = 0; x < lastWidth; x += 2) {
				const uint16_t *p00 = _pixel(x, y, lastWidth, lastHeight, lastData.data());
				const uint16_t *p10 = _pixel(x + 1, y, lastWidth, lastHeight, lastData.data());
				const uint16_t *p01 = _pixel(x, y + 1, lastWidth, lastHeight, lastData.data());
				const uint16_t *p11 = _pixel(x + 1, y + 1, lastWidth, lastHeight, lastData.data());

				float p0[4];
				for (int i = 0; i < 4; i++) {
					float value = _halfToFloat(p00[i]) + _halfToFloat(p10[i]);
					p0[i] = value / 2.0;
				}

				float p1[4];
				for (int i = 0; i < 4; i++) {
					float value = _halfToFloat(p01[i]) + _halfToFloat(p11[i]);
					p1[i] = value / 2.0;
				}

				float p[4];
				for (int i = 0; i < 4; i++) {
					p[i] = (p0[i] + p1[i]) / 2.0;
				}

				buffer.push_back(_floatToHalf(p[0]));
				buffer.push_back(_floatToHalf(p[1]));
				buffer.push_back(_floatToHalf(p[2]));
				buffer.push_back(_floatToHalf(p[3]));
			}
		}

		lastWidth = lastWidth >> 1;
		lastHeight = lastHeight >> 1;

		if (lastWidth < 1)
			lastWidth = 1;

		if (lastHeight < 1)
			lastHeight = 1;

		rd.imageSend(image.image, lastWidth, lastHeight, (uint8_t *)buffer.data(),
				buffer.size() * sizeof(uint16_t), vk::ImageLayout::eTransferDstOptimal, level);

		lastData = buffer;
	}
}

bool Compression::imageCompress(Image *pImage) {
	Image::Format format = pImage->getFormat();

	if (format != Image::Format::RGBAF16)
		return false;

	uint32_t width = pImage->getWidth();
	uint32_t height = pImage->getHeight();

	std::vector<uint8_t> data = pImage->getData();
	uint16_t *pData = reinterpret_cast<uint16_t *>(data.data());

	_generateMipmaps(width, height, data);

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

	{
		std::vector<float> data(pixelCount);
		for (uint32_t i = 0; i < pixelCount; i++) {
			data[i] = _halfToFloat(pData[i]);
		}

		vk::Format format = vk::Format::eR32G32B32A32Sfloat;

		AllocatedImage image =
				rd.imageCreate(width, height, format, 1, vk::ImageUsageFlagBits::eTransferDst);

		rd.imageLayoutTransition(image.image, format, 1, 1, vk::ImageLayout::eUndefined,
				vk::ImageLayout::eTransferDstOptimal);

		rd.imageSend(image.image, width, height, (uint8_t *)data.data(), pixelCount * sizeof(float),
				vk::ImageLayout::eTransferDstOptimal);
	}

	free(pBuffer);
	return true;
}
