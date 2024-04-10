#ifndef IMAGE_H
#define IMAGE_H

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

struct Color {
	uint8_t r, g, b = 0;
	uint8_t a = 255;

	Color average(const Color &c) const {
		uint16_t _r = r + c.r;
		uint16_t _g = g + c.g;
		uint16_t _b = b + c.b;
		uint16_t _a = a + c.a;

		return {
			static_cast<uint8_t>(_r >> 1),
			static_cast<uint8_t>(_g >> 1),
			static_cast<uint8_t>(_b >> 1),
			static_cast<uint8_t>(_a >> 1),
		};
	}

	uint8_t getLuminance() const {
		float _r = static_cast<float>(r) * 0.2126f;
		float _g = static_cast<float>(g) * 0.7152f;
		float _b = static_cast<float>(b) * 0.0722f;

		return std::round(_r + _g + _b);
	}
};

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
	static std::shared_ptr<Image> create(
			uint32_t width, uint32_t height, Format format, const std::vector<uint8_t> &data);

	uint32_t getWidth() const;
	uint32_t getHeight() const;

	Format getFormat() const;

	const std::vector<uint8_t> &getData() const;
	size_t getPixelSize() const;
};

#endif // !IMAGE_H
