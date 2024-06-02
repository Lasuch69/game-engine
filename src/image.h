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
		R = 0,
		G = 1,
		B = 2,
		A = 3,
	};

private:
	uint32_t _width, _height;
	Format _format = Format::R8;
	std::vector<uint8_t> _data = {};

public:
	Image *extractComponent(Channel channel) const;

	Image *getColorMap() const;
	Image *getNormalMap() const;
	Image *getMetallicMap(Channel channel) const;
	Image *getRoughnessMap(Channel channel) const;

	static size_t formatSize(Format format);
	size_t getFormatSize() const;

	uint32_t getWidth() const;
	uint32_t getHeight() const;
	Format getFormat() const;

	std::vector<uint8_t> getData() const;

	Image(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data);
};

#endif // !IMAGE_H
