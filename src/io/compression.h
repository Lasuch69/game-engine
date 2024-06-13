#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <cstddef>
#include <cstdint>

#include <math/float16.h>

typedef struct {
	size_t bufferLength;
	uint8_t *pBuffer;
} CompressionData;

class Compression {
public:
	static CompressionData imageCompress(uint32_t width, uint32_t height, const half *pData);
};

#endif // !COMPRESSION_H
