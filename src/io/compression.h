#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <cstdint>
#include <vector>

class Image;

class Compression {
	static std::vector<uint8_t> _compressStep(uint32_t width, uint32_t height, uint16_t *pData);

public:
	static bool imageCompress(Image *pImage);
};

#endif // !COMPRESSION_H
