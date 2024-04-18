#include <cmath>
#include <cstdint>
#include <vector>

#include "image.h"

uint8_t Color::getLuminance() const {
	float _r = static_cast<float>(r) * 0.2126f;
	float _g = static_cast<float>(g) * 0.7152f;
	float _b = static_cast<float>(b) * 0.0722f;

	return std::round(_r + _g + _b);
}

Image::Format Image::getFormatFromChannels(uint32_t channels) {
	switch (channels) {
		case 1:
			return Image::Format::R8;
		case 2:
			return Image::Format::RG8;
		case 3:
			return Image::Format::RGB8;
		case 4:
			return Image::Format::RGBA8;
	}

	return Image::Format::R8;
}

uint32_t Image::getChannelsFromFormat(Image::Format format) {
	switch (format) {
		case Image::Format::R8:
		case Image::Format::L8:
			return 1;
		case Image::Format::RG8:
		case Image::Format::LA8:
			return 2;
		case Image::Format::RGB8:
			return 3;
		case Image::Format::RGBA8:
			return 4;
	}

	return 1;
}

Color Image::getPixel(uint32_t idx) const {
	uint32_t i = idx * getChannelsFromFormat(_format);

	Color color = {};

	switch (_format) {
		case Format::R8:
		case Format::L8:
			color.r = _data[i + 0];
			break;
		case Format::RG8:
			color.r = _data[i + 0];
			color.g = _data[i + 1];
			break;
		case Format::LA8:
			color.r = _data[i + 0];
			color.a = _data[i + 1];
			break;
		case Format::RGB8:
			color.r = _data[i + 0];
			color.g = _data[i + 1];
			color.b = _data[i + 2];
			break;
		case Format::RGBA8:
			color.r = _data[i + 0];
			color.g = _data[i + 1];
			color.b = _data[i + 2];
			color.a = _data[i + 3];
			break;
	}

	return color;
}

void Image::setPixel(uint32_t idx, const Color &color) {
	uint32_t i = idx * getChannelsFromFormat(_format);

	switch (_format) {
		case Format::R8:
		case Format::L8:
			_data[i + 0] = color.r;
			break;
		case Format::RG8:
			_data[i + 0] = color.r;
			_data[i + 1] = color.g;
			break;
		case Format::LA8:
			_data[i + 0] = color.r;
			_data[i + 1] = color.a;
			break;
		case Format::RGB8:
			_data[i + 0] = color.r;
			_data[i + 1] = color.g;
			_data[i + 2] = color.b;
			break;
		case Format::RGBA8:
			_data[i + 0] = color.r;
			_data[i + 1] = color.g;
			_data[i + 2] = color.b;
			_data[i + 3] = color.a;
			break;
	}
}

Image *Image::_create(Format format) const {
	uint32_t pixelCount = _width * _height;
	uint32_t channels = getChannelsFromFormat(format);

	std::vector<uint8_t> data(pixelCount * channels);

	Image *pImage = new Image(_width, _height, format, data);

	for (uint32_t i = 0; i < pixelCount; i++) {
		Color c = getPixel(i);
		pImage->setPixel(i, c);
	}

	return pImage;
}

Image *Image::createR8() const {
	return _create(Format::R8);
}

Image *Image::createRG8() const {
	return _create(Format::RG8);
}

Image *Image::createRGBA8() const {
	return _create(Format::RGBA8);
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

const std::vector<uint8_t> &Image::getData() const {
	return _data;
}

Image::Image(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data) {
	_width = width;
	_height = height;
	_format = format;
	_data = data;
}

Image::Image(uint32_t width, uint32_t height, uint32_t channels, const std::vector<uint8_t> &data) {
	Image::Format format = getFormatFromChannels(channels);

	_width = width;
	_height = height;
	_format = format;
	_data = data;
}
