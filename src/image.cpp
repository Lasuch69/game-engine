#include <cstdint>
#include <vector>

#include "image.h"

size_t _formatPixelSize(Image::Format format) {
	switch (format) {
		case Image::Format::L8:
			return 1;
		case Image::Format::LA8:
			return 2;
		case Image::Format::RGB8:
			return 3;
		case Image::Format::RGBA8:
			return 4;
	}

	return 0;
}

Image::Image(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data) {
	_width = width;
	_height = height;
	_format = format;
	_data = data;
}

Image *Image::create(
		uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data) {
	size_t size = width * height * _formatPixelSize(format);

	if (size == data.size()) {
		return new Image(width, height, format, data);
	}

	return nullptr;
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

uint64_t Image::getPixelSize() const {
	return _formatPixelSize(_format);
}

const std::vector<uint8_t> &Image::getData() const {
	return _data;
}
