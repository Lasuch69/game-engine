#include <cstdint>
#include <vector>

#include "image.h"

typedef struct {
	float r, g, b, a;
} Color;

static Color _getPixel(const uint8_t *pBytes, const Image::Format &format, uint32_t idx) {
	uint32_t ofs = idx * Image::getFormatChannelCount(format);

	Color color = {};
	switch (format) {
		case Image::Format::R8:
			color.r = pBytes[ofs + 0] / 255.0;
			color.g = pBytes[ofs + 0] / 255.0;
			color.b = pBytes[ofs + 0] / 255.0;
			color.a = 1.0;
			break;
		case Image::Format::RG8:
			color.r = pBytes[ofs + 0] / 255.0;
			color.g = pBytes[ofs + 1] / 255.0;
			color.b = 0.0;
			color.a = 1.0;
			break;
		case Image::Format::RGB8:
			color.r = pBytes[ofs + 0] / 255.0;
			color.g = pBytes[ofs + 1] / 255.0;
			color.b = pBytes[ofs + 2] / 255.0;
			color.a = 1.0;
			break;
		case Image::Format::RGBA8:
			color.r = pBytes[ofs + 0] / 255.0;
			color.g = pBytes[ofs + 1] / 255.0;
			color.b = pBytes[ofs + 2] / 255.0;
			color.a = pBytes[ofs + 3] / 255.0;
			break;
		case Image::Format::RGBA32F:
			// uint8_t to float
			const float *pData = reinterpret_cast<const float *>(pBytes);

			color.r = pData[ofs + 0];
			color.g = pData[ofs + 1];
			color.b = pData[ofs + 2];
			color.a = pData[ofs + 3];
			break;
	}

	return color;
}

static void _setPixel(
		uint8_t *pBytes, const Image::Format &format, uint32_t idx, const Color &color) {
	uint32_t ofs = idx * Image::getFormatChannelCount(format);

	switch (format) {
		case Image::Format::R8:
			pBytes[ofs + 0] = color.r * 255;
			break;
		case Image::Format::RG8:
			pBytes[ofs + 0] = color.r * 255;
			pBytes[ofs + 1] = color.g * 255;
			break;
		case Image::Format::RGB8:
			pBytes[ofs + 0] = color.r * 255;
			pBytes[ofs + 1] = color.g * 255;
			pBytes[ofs + 2] = color.b * 255;
			break;
		case Image::Format::RGBA8:
			pBytes[ofs + 0] = color.r * 255;
			pBytes[ofs + 1] = color.g * 255;
			pBytes[ofs + 2] = color.b * 255;
			pBytes[ofs + 3] = color.a * 255;
			break;
		case Image::Format::RGBA32F:
			// uint8_t to float
			float *pData = reinterpret_cast<float *>(pBytes);

			pData[ofs + 0] = color.r;
			pData[ofs + 1] = color.g;
			pData[ofs + 2] = color.b;
			pData[ofs + 3] = color.a;
			break;
	}
}

uint32_t Image::getFormatByteSize(const Format &format) {
	switch (format) {
		case Image::Format::R8:
			return 1;
		case Image::Format::RG8:
			return 2;
		case Image::Format::RGB8:
			return 3;
		case Image::Format::RGBA8:
			return 4;
		case Image::Format::RGBA32F:
			return 16;
	}

	return 0;
}

uint32_t Image::getFormatChannelCount(const Format &format) {
	switch (format) {
		case Image::Format::R8:
			return 1;
		case Image::Format::RG8:
			return 2;
		case Image::Format::RGB8:
			return 3;
		case Image::Format::RGBA8:
		case Image::Format::RGBA32F:
			return 4;
	}

	return 0;
}

const char *Image::getFormatName(const Format &format) {
	switch (format) {
		case Image::Format::R8:
			return "R8";
		case Image::Format::RG8:
			return "RG8";
		case Image::Format::RGB8:
			return "RGB8";
		case Image::Format::RGBA8:
			return "RGBA8";
		case Image::Format::RGBA32F:
			return "RGBA32F";
	}

	return "";
}

void Image::convert(const Format &format) {
	uint32_t pixelCount = _width * _height;
	uint32_t byteSize = getFormatByteSize(format);

	std::vector<uint8_t> data(pixelCount * byteSize);

	uint8_t *pSrcData = _data.data();
	Format srcFormat = _format;

	uint8_t *pDstData = data.data();
	Format dstFormat = format;

	for (uint32_t pixelIdx = 0; pixelIdx < pixelCount; pixelIdx++) {
		Color color = _getPixel(pSrcData, srcFormat, pixelIdx);
		_setPixel(pDstData, dstFormat, pixelIdx, color);
	}

	_format = format;
	_data = data;
}

Image *Image::getComponent(const Channel &channel) const {
	uint32_t pixelCount = _width * _height;

	std::vector<uint8_t> data(pixelCount);

	const uint8_t *pSrcData = _data.data();
	Format srcFormat = _format;

	uint8_t *pDstData = data.data();
	Format dstFormat = Format::R8;

	for (size_t pixelIdx = 0; pixelIdx < pixelCount; pixelIdx++) {
		Color src = _getPixel(pSrcData, srcFormat, pixelIdx);
		Color color = {};

		// swizzle
		switch (channel) {
			case Channel::R:
				color.r = src.r;
				break;
			case Channel::G:
				color.r = src.g;
				break;
			case Channel::B:
				color.r = src.b;
				break;
			case Channel::A:
				color.r = src.a;
				break;
		}

		_setPixel(pDstData, dstFormat, pixelIdx, color);
	}

	return new Image(_width, _height, Format::R8, data);
}

uint32_t Image::getWidth() const {
	return _width;
}

uint32_t Image::getHeight() const {
	return _height;
}

Image::Format Image::getFormat() const {
	return _format;
}

std::vector<uint8_t> Image::getData() const {
	return _data;
}

Image::Image(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data) {
	_width = width;
	_height = height;
	_format = format;
	_data = data;
}
