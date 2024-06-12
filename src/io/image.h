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

public:
	static uint32_t getFormatByteSize(Format format) {
		switch (format) {
			case Image::Format::R8:
				return 1;
			case Image::Format::RG8:
				return 2;
			case Image::Format::RGB8:
				return 3;
			case Image::Format::RGBA8:
				return 4;
			case Image::Format::RGBAF16:
				return 8;
			default:
				break;
		}

		return 0;
	};

	static uint32_t getFormatChannelCount(Format format) {
		switch (format) {
			case Image::Format::R8:
				return 1;
			case Image::Format::RG8:
				return 2;
			case Image::Format::RGB8:
				return 3;
			case Image::Format::RGBA8:
			case Image::Format::RGBAF16:
				return 4;
			default:
				break;
		}

		return 0;
	}

	static const char *getFormatName(Format format) {
		switch (format) {
			case Image::Format::R8:
				return "R8";
			case Image::Format::RG8:
				return "RG8";
			case Image::Format::RGB8:
				return "RGB8";
			case Image::Format::RGBA8:
				return "RGBA8";
			case Image::Format::RGBAF16:
				return "RGBAF16";
			case Image::Format::BC6HS:
				return "BC6HS";
			case Image::Format::BC6HU:
				return "BC6HU";
		}

		return "";
	}
};

#endif // !IMAGE_H
