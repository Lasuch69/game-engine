#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <vector>

struct Color {
	uint8_t r, g, b = 0;
	uint8_t a = 255;

	uint8_t getLuminance() const;
};

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

private:
	uint32_t _width, _height = 0;
	Format _format = Format::L8;
	std::vector<uint8_t> _data = {};

	Image *_create(Format format) const;

public:
	static Image::Format getFormatFromChannels(uint32_t channels);
	static uint32_t getChannelsFromFormat(Image::Format format);

	Color getPixel(uint32_t idx) const;
	void setPixel(uint32_t idx, const Color &color);

	Image *createR8() const;
	Image *createRG8() const;
	Image *createRGBA8() const;

	uint32_t getWidth() const;
	uint32_t getHeight() const;
	Format getFormat() const;

	const std::vector<uint8_t> &getData() const;

	Image(uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data);
	Image(uint32_t width, uint32_t height, uint32_t channels, const std::vector<uint8_t> &data);
};

#endif // !IMAGE_H
