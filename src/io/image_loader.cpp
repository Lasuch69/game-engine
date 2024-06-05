#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <stb/stb_image.h>
#include <tinyexr/tinyexr.h>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>

#include "image_loader.h"

#define STBI_FAILURE 0
#define STBI_SUCCESS 1

void ImageLoader::_printInfo(const Image *pImage, const char *pFile) {
	const SDL_LogCategory CATEGORY = SDL_LOG_CATEGORY_APPLICATION;

	if (pImage == nullptr) {
		if (pFile == nullptr) {
			SDL_LogError(CATEGORY, "Loading image from memory failed");
			return;
		}

		SDL_LogError(CATEGORY, "Loading image from path (%s) failed", pFile);
		return;
	}

	if (pFile == nullptr) {
		SDL_Log("Loaded image from memory");
	} else {
		SDL_Log("Loaded image from path: %s", pFile);
	}

	const char *pFormat = "";

	switch (pImage->getFormat()) {
		case Image::Format::R8:
			pFormat = "R8";
			break;
		case Image::Format::RG8:
			pFormat = "RG8";
			break;
		case Image::Format::RGB8:
			pFormat = "RGB8";
			break;
		case Image::Format::RGBA8:
			pFormat = "RGBA8";
			break;
		case Image::Format::RGBA32F:
			pFormat = "RGBA32F";
			break;
	}

	SDL_LogVerbose(CATEGORY, "Width: %d", pImage->getWidth());
	SDL_LogVerbose(CATEGORY, "Height: %d", pImage->getHeight());
	SDL_LogVerbose(CATEGORY, "Format: %s", pFormat);
	SDL_LogVerbose(CATEGORY, "Bytes: %ld", pImage->getData().size());
}

Image *ImageLoader::_stbiLoad(const uint8_t *pBuffer, size_t bufferSize) {
	int width, height, numChannels;
	stbi_uc *pData = stbi_load_from_memory(pBuffer, bufferSize, &width, &height, &numChannels, 0);

	if (pData == nullptr)
		return nullptr;

	size_t byteSize = width * height * numChannels;

	std::vector<uint8_t> bytes(byteSize);
	memcpy(bytes.data(), pData, byteSize);
	stbi_image_free(pData);

	Image::Format format = Image::Format::R8;

	switch (numChannels) {
		case 1:
			format = Image::Format::R8;
			break;
		case 2:
			format = Image::Format::RG8;
			break;
		case 3:
			format = Image::Format::RGB8;
			break;
		case 4:
			format = Image::Format::RGBA8;
			break;
	}

	return new Image(width, height, format, bytes);
}

Image *ImageLoader::_stbiLoadHDR(const uint8_t *pBuffer, size_t bufferSize) {
	int width, height, numChannels;
	float *pData = stbi_loadf_from_memory(pBuffer, bufferSize, &width, &height, &numChannels, 0);

	if (pData == nullptr)
		return nullptr;

	uint32_t pixelCount = width * height;
	std::vector<float> data(pixelCount * 4);

	for (uint32_t pixel = 0; pixel < pixelCount; pixel++) {
		float channels[4] = { 0.0, 0.0, 0.0, 1.0 };

		for (int i = 0; i < numChannels; i++) {
			size_t offset = pixel * numChannels;
			channels[i] = pData[offset + i];
		}

		size_t offset = (pixel * 4);

		data[offset + 0] = channels[0];
		data[offset + 1] = channels[1];
		data[offset + 2] = channels[2];
		data[offset + 3] = channels[3];
	}

	size_t byteSize = data.size() * sizeof(float);

	std::vector<uint8_t> bytes(byteSize);
	memcpy(bytes.data(), data.data(), byteSize);
	stbi_image_free(pData);

	return new Image(width, height, Image::Format::RGBA32F, bytes);
}

Image *ImageLoader::_tinyexrLoad(const uint8_t *pBuffer, size_t bufferSize) {
	EXRVersion version;
	int err = ParseEXRVersionFromMemory(&version, pBuffer, bufferSize);

	if (err != TINYEXR_SUCCESS)
		return nullptr;

	// multipart EXR is unsupported
	if (version.multipart)
		return nullptr;

	EXRHeader header;
	InitEXRHeader(&header);

	err = ParseEXRHeaderFromMemory(&header, &version, pBuffer, bufferSize, nullptr);

	if (err != TINYEXR_SUCCESS)
		return nullptr;

	// read 16-bit float as 32-bit
	for (int i = 0; i < header.num_channels; i++) {
		if (header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF)
			header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
	}

	EXRImage image;
	InitEXRImage(&image);
	err = LoadEXRImageFromMemory(&image, &header, pBuffer, bufferSize, nullptr);

	if (err != TINYEXR_SUCCESS) {
		FreeEXRHeader(&header);
		return nullptr;
	}

	const int MAX_CHANNELS = 4;
	if (image.num_channels > MAX_CHANNELS) {
		FreeEXRImage(&image);
		FreeEXRHeader(&header);
		return nullptr;
	}

	if (image.images == nullptr) {
		FreeEXRImage(&image);
		FreeEXRHeader(&header);
		return nullptr;
	}

	// load

	uint32_t width = image.width;
	uint32_t height = image.height;

	uint32_t pixelCount = width * height;
	const float *const *pData = reinterpret_cast<float **>(image.images);

	std::vector<float> data(pixelCount * 4);

	for (uint32_t pixel = 0; pixel < pixelCount; pixel++) {
		// default alpha is 1.0
		float channels[4] = { 0.0, 0.0, 0.0, 1.0 };

		int idx = 0;

		// channels are in reverse order
		for (int i = image.num_channels - 1; i >= 0; i -= 1) {
			channels[idx] = pData[i][pixel];
			idx++;
		}

		size_t offset = (pixel * 4);

		data[offset + 0] = channels[0];
		data[offset + 1] = channels[1];
		data[offset + 2] = channels[2];
		data[offset + 3] = channels[3];
	}

	FreeEXRImage(&image);
	FreeEXRHeader(&header);

	size_t byteSize = data.size() * sizeof(float);
	std::vector<uint8_t> bytes(byteSize);
	memcpy(bytes.data(), data.data(), byteSize);

	return new Image(width, height, Image::Format::RGBA32F, bytes);
}

bool ImageLoader::isImage(const char *pFile) {
	size_t bufferSize;
	uint8_t *pBuffer = static_cast<uint8_t *>(SDL_LoadFile(pFile, &bufferSize));

	int w, h, c;
	int result = stbi_info_from_memory(pBuffer, bufferSize, &w, &h, &c);

	if (result == STBI_SUCCESS)
		return true;

	if (IsEXRFromMemory(pBuffer, bufferSize) == TINYEXR_SUCCESS)
		return true;

	return false;
}

std::shared_ptr<Image> ImageLoader::loadFromFile(const char *pFile) {
	size_t bufferSize;
	uint8_t *pBuffer = static_cast<uint8_t *>(SDL_LoadFile(pFile, &bufferSize));

	int w, h, c;
	int result = stbi_info_from_memory(pBuffer, bufferSize, &w, &h, &c);

	Image *pImage = nullptr;

	if (result == STBI_SUCCESS) {
		if (stbi_is_hdr_from_memory(pBuffer, bufferSize)) {
			pImage = _stbiLoadHDR(pBuffer, bufferSize);
		} else {
			pImage = _stbiLoad(pBuffer, bufferSize);
		}
	} else {
		pImage = _tinyexrLoad(pBuffer, bufferSize);
	}

	_printInfo(pImage, pFile);
	return std::shared_ptr<Image>(pImage);
}

std::shared_ptr<Image> ImageLoader::loadFromMemory(const uint8_t *pBuffer, size_t bufferSize) {
	int w, h, c;
	int result = stbi_info_from_memory(pBuffer, bufferSize, &w, &h, &c);

	Image *pImage = nullptr;

	if (result == STBI_SUCCESS) {
		pImage = _stbiLoad(pBuffer, bufferSize);
	} else {
		pImage = _tinyexrLoad(pBuffer, bufferSize);
	}

	_printInfo(pImage, nullptr);
	return std::shared_ptr<Image>(pImage);
}
