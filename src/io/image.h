#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <vector>

class Image {
public:
	enum class Format {
		R8,
		RG8,
		RGB8,
		RGBA8,
		RGBA32F,
	};

	enum class Channel {
		R,
		G,
		B,
		A,
	};

private:
	uint32_t _width, _height;
	Format _format = Format::R8;
	std::vector<uint8_t> _data = {};

	struct Color {
		uint8_t r, g, b = 0;
		uint8_t a = 255;
	};

	Color _getPixelAtOffset(size_t offset) const;
	void _setPixelAtOffset(size_t offset, const Color &color);

public:
	Image *getColorMap() const;
	Image *getNormalMap() const;
	Image *getMetallicMap(Channel channel) const;
	Image *getRoughnessMap(Channel channel) const;

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	Format getFormat() const;
	uint32_t getPixelSize() const;

	std::vector<uint8_t> getData() const;

	Image(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data);
};

#endif // !IMAGE_H
