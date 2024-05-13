#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <vector>

class Image {
public:
	enum class Format {
		R8,
		L8,
		RG8,
		LA8,
		RGB8,
		RGBA8,
	};

	enum class Channel {
		R,
		G,
		B,
		A,
	};

private:
	uint32_t _width, _height;
	Format _format = Format::L8;
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
	std::vector<uint8_t> getData() const;

	uint32_t getPixelSize() const;

	Image(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data);
	Image(uint32_t width, uint32_t height, uint32_t channels, const std::vector<uint8_t> &data);
};

#endif // !IMAGE_H
