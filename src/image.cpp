#include <cstdint>
#include <vector>

#include "image.h"

Image *Image::extractComponent(Channel channel) const {
	if (_format == Image::Format::RGBA32F)
		return nullptr;

	size_t pixelCount = _width * _height;
	std::vector<uint8_t> data(pixelCount);

	uint32_t pixelSize = getFormatSize();
	uint32_t componentIdx = static_cast<uint32_t>(channel);

	if (componentIdx < pixelSize) {
		for (size_t i = 0; i < pixelCount; i++) {
			data[i] = _data[i * pixelSize + componentIdx];
		}
	} else {
		bool isAlpha = channel == Channel::A;

		if (isAlpha) {
			data.resize(pixelCount, 255);
		} else {
			data.resize(pixelCount, 0);
		}
	}

	return new Image(_width, _height, Format::R8, data);
}

Image *Image::getColorMap() const {
	if (_format == Image::Format::RGBA32F)
		return nullptr;

	size_t pixelCount = _width * _height;
	std::vector<uint8_t> data(pixelCount * 4);

	uint32_t ofs = 0;

	for (size_t i = 0; i < pixelCount * 4; i += 4) {
		switch (_format) {
			case Format::R8:
				data[i + 0] = _data[ofs + 0];
				data[i + 1] = _data[ofs + 0];
				data[i + 2] = _data[ofs + 0];
				data[i + 3] = 255;
				break;
			case Format::RG8:
				data[i + 0] = _data[ofs + 0];
				data[i + 1] = _data[ofs + 1];
				data[i + 2] = 0;
				data[i + 3] = 255;
				break;
			case Format::RGB8:
				data[i + 0] = _data[ofs + 0];
				data[i + 1] = _data[ofs + 1];
				data[i + 2] = _data[ofs + 2];
				data[i + 3] = 255;
				break;
			case Format::RGBA8:
				data[i + 0] = _data[ofs + 0];
				data[i + 1] = _data[ofs + 1];
				data[i + 2] = _data[ofs + 2];
				data[i + 3] = _data[ofs + 3];
				break;
			default:
				continue;
		}

		ofs += getFormatSize();
	}

	return new Image(_width, _height, Format::RGBA8, data);
}

Image *Image::getNormalMap() const {
	if (_format == Image::Format::RGBA32F)
		return nullptr;

	size_t pixelCount = _width * _height;
	std::vector<uint8_t> data(pixelCount * 2);

	uint32_t ofs = 0;

	for (size_t i = 0; i < pixelCount * 2; i += 2) {
		switch (_format) {
			case Format::R8:
				data[i + 0] = _data[ofs + 0];
				data[i + 1] = _data[ofs + 0];
				break;
			case Format::RG8:
				data[i + 0] = _data[ofs + 0];
				data[i + 1] = _data[ofs + 1];
				break;
			case Format::RGB8:
				data[i + 0] = _data[ofs + 0];
				data[i + 1] = _data[ofs + 1];
				break;
			case Format::RGBA8:
				data[i + 0] = _data[ofs + 0];
				data[i + 1] = _data[ofs + 1];
				break;
			default:
				continue;
		}

		ofs += getFormatSize();
	}

	return new Image(_width, _height, Format::RG8, data);
}

Image *Image::getMetallicMap(Channel channel) const {
	return extractComponent(channel);
}

Image *Image::getRoughnessMap(Channel channel) const {
	return extractComponent(channel);
}

size_t Image::formatSize(Image::Format format) {
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

	return 1;
}

size_t Image::getFormatSize() const {
	return formatSize(_format);
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
