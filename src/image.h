#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <vector>

class Image {
public:
	enum class Format {
		L8,
		LA8,
		RGB8,
		RGBA8,
	};

private:
	uint32_t _width, _height = 0;
	Format _format = Format::L8;
	std::vector<uint8_t> _data = {};

	Image(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data);

public:
	static Image *create(
			uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data);

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	Format getFormat() const;

	const std::vector<uint8_t> &getData() const;
	size_t getPixelSize() const;
};

#endif // !IMAGE_H
