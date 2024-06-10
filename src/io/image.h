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
		RGBAF16,
		BC6HS,
		BC6HU,
	};

	enum class Channel {
		R,
		G,
		B,
		A,
	};

private:
	uint32_t _width, _height;
	uint32_t _mipLevels = 1;
	Format _format = Format::R8;
	std::vector<uint8_t> _data = {};

public:
	static uint32_t getFormatByteSize(Format format);
	static uint32_t getFormatChannelCount(Format format);
	static const char *getFormatName(Format format);

	bool isCompressed() const;

	bool convert(Format format);
	Image *getComponent(Channel channel) const;

	bool generateMipmaps();

	uint32_t getWidth() const;
	uint32_t getHeight() const;
	uint32_t getMipLevels() const;
	Format getFormat() const;
	std::vector<uint8_t> getData() const;

	Image(uint32_t width, uint32_t height, uint32_t mipLevels, Format format,
			std::vector<uint8_t> data);
};

#endif // !IMAGE_H
