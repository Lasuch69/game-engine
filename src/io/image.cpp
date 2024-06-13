#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include <math/float16.h>
#include <math/util.h>

#include "image.h"
#include "io/compression.h"

typedef struct {
	uint32_t width, height;
	size_t bufferLength;
	uint8_t *pBuffer;
} Level;

typedef struct {
	uint32_t levelCount;
	Level *pLevels;
} MipmapData;

typedef struct {
	uint8_t channels[4];
} Color8;

typedef struct {
	half channels[4];
} ColorF16;

static uint32_t getLevelCount(uint32_t width, uint32_t height) {
	uint32_t levelCount = 0;
	while (width > 1 || height > 1) {
		width = max<uint32_t>(width >> 1, 1);
		height = max<uint32_t>(height >> 1, 1);

		levelCount++;
	}

	return levelCount;
}

static Color8 getPixel8(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
		uint32_t channelCount, const uint8_t *pBuffer) {
	x = min<uint32_t>(x, width - 1);
	y = min<uint32_t>(y, height - 1);

	Color8 color;
	color.channels[3] = 255;

	uint32_t offset = ((y * width) + x) * channelCount;
	for (uint32_t i = 0; i < channelCount; i++)
		color.channels[i] = pBuffer[offset + i];

	return color;
}

static MipmapData mipmapsGenerateU8(
		uint32_t width, uint32_t height, uint32_t channels, uint8_t *pData) {
	uint32_t levelCount = getLevelCount(width, height);

	uint32_t lastWidth = width;
	uint32_t lastHeight = height;

	uint8_t *pLastBuffer = pData;

	MipmapData mipmapData;
	mipmapData.levelCount = levelCount;
	mipmapData.pLevels = new Level[levelCount];

	for (uint32_t i = 0; i < levelCount; i++) {
		uint32_t mipmapWidth = max<uint32_t>(lastWidth >> 1, 1);
		uint32_t mipmapHeight = max<uint32_t>(lastHeight >> 1, 1);

		size_t bufferLength = mipmapWidth * mipmapHeight * channels;
		uint8_t *pBuffer = new uint8_t[bufferLength];

		uint32_t idx = 0;
		for (uint32_t y = 0; y < lastHeight; y += 2) {
			for (uint32_t x = 0; x < lastWidth; x += 2) {
				Color8 p00 = getPixel8(x, y, lastWidth, lastHeight, channels, pLastBuffer);
				Color8 p10 = getPixel8(x + 1, y, lastWidth, lastHeight, channels, pLastBuffer);
				Color8 p01 = getPixel8(x, y + 1, lastWidth, lastHeight, channels, pLastBuffer);
				Color8 p11 = getPixel8(x + 1, y + 1, lastWidth, lastHeight, channels, pLastBuffer);

				for (uint32_t i = 0; i < channels; i++) {
					uint16_t v0 = p00.channels[i] + p01.channels[i];
					uint16_t v1 = p10.channels[i] + p11.channels[i];
					pBuffer[idx] = (v0 + v1) >> 2;
					idx++;
				}
			}
		}

		Level level;
		level.width = mipmapWidth;
		level.height = mipmapHeight;
		level.bufferLength = bufferLength;
		level.pBuffer = pBuffer;

		mipmapData.pLevels[i] = level;

		lastWidth = mipmapWidth;
		lastHeight = mipmapHeight;

		pLastBuffer = pBuffer;
	}

	return mipmapData;
}

static ColorF16 getPixelF16(
		uint32_t x, uint32_t y, uint32_t width, uint32_t height, const half *pBuffer) {
	x = min<uint32_t>(x, width - 1);
	y = min<uint32_t>(y, height - 1);

	uint32_t offset = ((y * width) + x) * 4;

	ColorF16 color;
	color.channels[0] = pBuffer[offset + 0];
	color.channels[1] = pBuffer[offset + 1];
	color.channels[2] = pBuffer[offset + 2];
	color.channels[3] = pBuffer[offset + 3];

	return color;
}

static MipmapData mipmapsGenerateF16(uint32_t width, uint32_t height, uint8_t *pData) {
	uint32_t levelCount = getLevelCount(width, height);

	uint32_t lastWidth = width;
	uint32_t lastHeight = height;

	half *pLastBuffer = reinterpret_cast<half *>(pData);

	MipmapData mipmapData;
	mipmapData.levelCount = levelCount;
	mipmapData.pLevels = new Level[levelCount];

	const uint32_t CHANNEL_COUNT = 4;

	for (uint32_t i = 0; i < levelCount; i++) {
		uint32_t mipmapWidth = max<uint32_t>(lastWidth >> 1, 1);
		uint32_t mipmapHeight = max<uint32_t>(lastHeight >> 1, 1);

		size_t bufferLength = mipmapWidth * mipmapHeight * CHANNEL_COUNT;
		half *pBuffer = new half[bufferLength];

		uint32_t idx = 0;
		for (uint32_t y = 0; y < lastHeight; y += 2) {
			for (uint32_t x = 0; x < lastWidth; x += 2) {
				ColorF16 p00 = getPixelF16(x, y, lastWidth, lastHeight, pLastBuffer);
				ColorF16 p10 = getPixelF16(x + 1, y, lastWidth, lastHeight, pLastBuffer);
				ColorF16 p01 = getPixelF16(x, y + 1, lastWidth, lastHeight, pLastBuffer);
				ColorF16 p11 = getPixelF16(x + 1, y + 1, lastWidth, lastHeight, pLastBuffer);

				// FIXME: halfAverageApprox() is not suitable for negative values
				for (int i = 0; i < CHANNEL_COUNT; i++) {
					half h0 = halfAverageApprox(p00.channels[i], p10.channels[i]);
					half h1 = halfAverageApprox(p01.channels[i], p11.channels[i]);
					pBuffer[idx] = halfAverageApprox(h0, h1);
					idx++;
				}
			}
		}

		Level level;
		level.width = mipmapWidth;
		level.height = mipmapHeight;
		level.bufferLength = bufferLength * sizeof(half);
		level.pBuffer = reinterpret_cast<uint8_t *>(pBuffer);

		mipmapData.pLevels[i] = level;

		lastWidth = mipmapWidth;
		lastHeight = mipmapHeight;

		pLastBuffer = pBuffer;
	}

	return mipmapData;
}

bool Image::compress() {
	if (isCompressed())
		return false;

	if (_format != Format::RGBAF16)
		return false;

	half *pData = reinterpret_cast<half *>(_data.data());

	uint32_t width = _width;
	uint32_t height = _height;
	uint32_t channels = getFormatChannelCount(_format);
	uint32_t levelCount = _mipLevels;

	CompressionData *pLevels = new CompressionData[levelCount];

	size_t offset = 0;
	size_t byteSize = 0;

	for (uint32_t i = 0; i < levelCount; i++) {
		pLevels[i] = Compression::imageCompress(width, height, &pData[offset]);
		offset += width * height * channels;
		byteSize += pLevels[i].bufferLength;

		width = max<uint32_t>(width >> 1, 1);
		height = max<uint32_t>(height >> 1, 1);
	}

	std::vector<uint8_t> data(byteSize);

	offset = 0;
	for (uint32_t i = 0; i < levelCount; i++) {
		CompressionData level = pLevels[i];
		memcpy(&data[offset], level.pBuffer, level.bufferLength);
		offset += level.bufferLength;

		free(level.pBuffer);
	}

	free(pLevels);

	_data = data;
	_format = Format::BC6HU;
	return true;
}

bool Image::isCompressed() const {
	switch (_format) {
		case Image::Format::BC6HS:
		case Image::Format::BC6HU:
			return true;
		default:
			break;
	}

	return false;
}

bool Image::convert(Format format) {
	if (isCompressed())
		return false;

	uint32_t pixelCount = _width * _height;

	uint32_t srcChannelCount = getFormatChannelCount(_format);
	uint32_t dstChannelCount = getFormatChannelCount(format);

	std::vector<uint8_t> data(pixelCount * dstChannelCount);

	for (uint32_t i = 0; i < pixelCount; i++) {
		uint8_t channels[4] = { 0, 0, 0, 255 };

		for (uint32_t channel = 0; channel < srcChannelCount; channel++)
			channels[channel] = _data[i * srcChannelCount];

		for (uint32_t channel = 0; channel < dstChannelCount; channel++)
			data[i * dstChannelCount] = channels[channel];
	}

	_format = format;
	_data = data;
	return true;
}

Image *Image::getComponent(Channel channel) const {
	if (isCompressed())
		return nullptr;

	uint32_t pixelCount = _width * _height;

	uint32_t srcChannelCount = getFormatChannelCount(_format);
	uint32_t dstChannelCount = 1;

	std::vector<uint8_t> data(pixelCount * dstChannelCount);

	for (uint32_t i = 0; i < pixelCount; i++) {
		uint8_t channels[4] = { 0, 0, 0, 255 };

		for (uint32_t channel = 0; channel < srcChannelCount; channel++)
			channels[channel] = _data[i * srcChannelCount];

		data[i] = channels[static_cast<uint32_t>(channel)];
	}

	return new Image(_width, _height, 1, Format::R8, data);
}

bool Image::mipmapsGenerate() {
	if (isCompressed())
		return false;

	MipmapData mipmaps;

	if (_format == Format::RGBAF16) {
		mipmaps = mipmapsGenerateF16(_width, _height, _data.data());
	} else {
		uint32_t channelCount = getFormatChannelCount(_format);
		mipmaps = mipmapsGenerateU8(_width, _height, channelCount, _data.data());
	}

	size_t byteOffset = _data.size();
	size_t newSize = _data.size();

	for (uint32_t i = 0; i < mipmaps.levelCount; i++) {
		Level level = mipmaps.pLevels[i];
		newSize += level.bufferLength;
	}

	_data.resize(newSize);

	for (uint32_t i = 0; i < mipmaps.levelCount; i++) {
		Level level = mipmaps.pLevels[i];
		memcpy(&_data[byteOffset], level.pBuffer, level.bufferLength);
		free(level.pBuffer);

		byteOffset += level.bufferLength;
	}

	free(mipmaps.pLevels);

	_mipLevels = mipmaps.levelCount + 1;
	return true;
}

uint32_t Image::getWidth() const {
	return _width;
}

uint32_t Image::getHeight() const {
	return _height;
}

uint32_t Image::getMipLevels() const {
	return _mipLevels;
}

Image::Format Image::getFormat() const {
	return _format;
}

std::vector<uint8_t> Image::getData() const {
	return _data;
}

Image::Image(uint32_t width, uint32_t height, uint32_t mipLevels, Format format,
		std::vector<uint8_t> data) {
	_width = width;
	_height = height;
	_mipLevels = mipLevels;
	_format = format;
	_data = data;
}
