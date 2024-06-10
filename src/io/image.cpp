#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "image.h"

uint32_t Image::getFormatByteSize(Format format) {
	switch (format) {
		case Image::Format::R8:
			return 1;
		case Image::Format::RG8:
			return 2;
		case Image::Format::RGB8:
			return 3;
		case Image::Format::RGBA8:
			return 4;
		case Image::Format::RGBAF16:
			return 8;
		default:
			break;
	}

	return 0;
}

uint32_t Image::getFormatChannelCount(Format format) {
	switch (format) {
		case Image::Format::R8:
			return 1;
		case Image::Format::RG8:
			return 2;
		case Image::Format::RGB8:
			return 3;
		case Image::Format::RGBA8:
		case Image::Format::RGBAF16:
			return 4;
		default:
			break;
	}

	return 0;
}

const char *Image::getFormatName(Format format) {
	switch (format) {
		case Image::Format::R8:
			return "R8";
		case Image::Format::RG8:
			return "RG8";
		case Image::Format::RGB8:
			return "RGB8";
		case Image::Format::RGBA8:
			return "RGBA8";
		case Image::Format::RGBAF16:
			return "RGBAF16";
		case Image::Format::BC6HS:
			return "BC6HS";
		case Image::Format::BC6HU:
			return "BC6HU";
	}

	return "";
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

typedef struct {
	uint8_t channels[4];
} Color8;

static uint32_t max(uint32_t a, uint32_t b) {
	return (a > b) ? a : b;
}

static Color8 getPixel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t channels,
		const std::vector<uint8_t> &data) {
	if (x >= w)
		x = w - 1;
	if (y >= h)
		y = h - 1;

	Color8 color;
	color.channels[3] = 255;

	uint32_t offset = ((y * w) + x) * channels;
	for (uint32_t i = 0; i < channels; i++)
		color.channels[i] = data[offset + i];

	return color;
}

bool Image::generateMipmaps() {
	uint32_t lastWidth = _width;
	uint32_t lastHeight = _height;
	std::vector<uint8_t> lastData = _data;

	uint32_t channels = getFormatChannelCount(_format);
	size_t offset = _data.size();

	printf("width: %d, height: %d\n", _width, _height);
	printf("size: %ld\n", _data.size());

	while (lastWidth > 1 || lastHeight > 1) {
		uint32_t mipmapWidth = max(lastWidth >> 1, 1);
		uint32_t mipmapHeight = max(lastHeight >> 1, 1);
		std::vector<uint8_t> mipmapData(mipmapWidth * mipmapHeight * channels);

		uint32_t idx = 0;
		for (uint32_t y = 0; y < lastHeight; y += 2) {
			for (uint32_t x = 0; x < lastWidth; x += 2) {
				Color8 p00 = getPixel(x, y, lastWidth, lastHeight, channels, lastData);
				Color8 p10 = getPixel(x + 1, y, lastWidth, lastHeight, channels, lastData);
				Color8 p01 = getPixel(x, y + 1, lastWidth, lastHeight, channels, lastData);
				Color8 p11 = getPixel(x + 1, y + 1, lastWidth, lastHeight, channels, lastData);

				for (uint32_t i = 0; i < channels; i++) {
					uint16_t v0 = p00.channels[i] + p01.channels[i];
					uint16_t v1 = p10.channels[i] + p11.channels[i];

					mipmapData[idx] = (v0 + v1) >> 2;
					idx++;
				}
			}
		}

		printf("width: %d, height: %d\n", mipmapWidth, mipmapHeight);
		printf("size: %ld\n", mipmapData.size());

		size_t newSize = _data.size() + mipmapData.size();
		_data.resize(newSize);
		memcpy(&_data[offset], mipmapData.data(), mipmapData.size());

		lastWidth = mipmapWidth;
		lastHeight = mipmapHeight;
		lastData = mipmapData;
	}

	return false;
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
