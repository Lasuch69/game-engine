#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include <cstdint>
#include <filesystem>

#include "image.h"

class ImageLoader {
public:
	static Image *loadFromFile(const std::filesystem::path &path);
	static Image *loadFromMemory(const std::vector<uint8_t> &bytes);

	static Image *loadHDRFromFile(const std::filesystem::path &path);
	static Image *loadEXRFromFile(const std::filesystem::path &path);
};

#endif // !IMAGE_LOADER_H
