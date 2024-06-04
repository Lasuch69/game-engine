#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <cstdint>
#include <memory>

#include "image.h"

class ImageLoader {
private:
	static void _printInfo(const Image *pImage, const char *pFile);

	static Image *_stbiLoad(const uint8_t *pBuffer, size_t bufferSize);
	static Image *_stbiLoadHDR(const uint8_t *pBuffer, size_t bufferSize);
	static Image *_tinyexrLoad(const uint8_t *pBuffer, size_t bufferSize);

public:
	static bool isImage(const char *pFile);

	static std::shared_ptr<Image> loadFromFile(const char *pFile);
	static std::shared_ptr<Image> loadFromMemory(const uint8_t *pBuffer, size_t bufferSize);
};

#endif // !IMAGE_LOADER_H
