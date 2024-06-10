#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <stb/stb_image.h>
#include <tinyexr/tinyexr.h>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>

#include "image_loader.h"

#define FLOAT16_ONE 0x3c00

#define FLOAT16_MAX 0x7bff // 01111011 11111111 =  65504.0
#define FLOAT16_MIN 0xfbff // 11111011 11111111 = -65504.0

#define STBI_FAILURE 0
#define STBI_SUCCESS 1

uint16_t _floatToHalf(float value) {
	union {
		float value;
		uint32_t bits;
	} data;

	data.value = value;
	uint32_t bits = data.bits;

	uint32_t sign = (bits >> 31) & 0x1;
	uint32_t exp = (bits >> 23) & 0xff;
	uint32_t frac = bits & 0x7fffff;

	bool isNegative = sign == 1;

	if (exp == 0xff) {
		// NaN
		if (frac > 0)
			return 0;

		// infinity
		return isNegative ? FLOAT16_MIN : FLOAT16_MAX;
	}

	exp = exp - 127 + 15;
	if (exp >= 0x1f)
		return isNegative ? FLOAT16_MIN : FLOAT16_MAX;

	frac = frac >> 13;
	return (sign << 15) + (exp << 10) + frac;
}

/*
float _halfToFloat(uint16_t value) {
	uint32_t sign = (value >> 15) & 0x1;
	uint32_t exp = (value >> 10) & 0x1f;
	uint32_t frac = value & 0x3ff;

	sign = sign << 31;
	exp = exp + 127 - 15;
	frac = frac << 13;

	union {
		float value;
		uint32_t bits;
	} data;

	data.bits = sign + exp + frac;
	return data.value;
}
*/

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

	Image::Format format = pImage->getFormat();

	SDL_LogVerbose(CATEGORY, "Width: %d", pImage->getWidth());
	SDL_LogVerbose(CATEGORY, "Height: %d", pImage->getHeight());
	SDL_LogVerbose(CATEGORY, "Format: %s", Image::getFormatName(format));
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

	return new Image(width, height, 1, format, bytes);
}

Image *ImageLoader::_stbiLoadHDR(const uint8_t *pBuffer, size_t bufferSize) {
	int width, height, numChannels;
	float *pData = stbi_loadf_from_memory(pBuffer, bufferSize, &width, &height, &numChannels, 0);

	if (pData == nullptr)
		return nullptr;

	uint32_t pixelCount = width * height;
	std::vector<uint16_t> data(pixelCount * 4);

	for (uint32_t pixel = 0; pixel < pixelCount; pixel++) {
		uint16_t channels[4];
		channels[3] = FLOAT16_ONE;

		for (int i = 0; i < numChannels; i++) {
			size_t offset = pixel * numChannels;
			float value = pData[offset + i];
			channels[i] = _floatToHalf(value);
		}

		size_t offset = (pixel * 4);

		data[offset + 0] = channels[0];
		data[offset + 1] = channels[1];
		data[offset + 2] = channels[2];
		data[offset + 3] = channels[3];
	}

	size_t byteSize = data.size() * sizeof(uint16_t);

	std::vector<uint8_t> bytes(byteSize);
	memcpy(bytes.data(), data.data(), byteSize);
	stbi_image_free(pData);

	return new Image(width, height, 1, Image::Format::RGBAF16, bytes);
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

	std::vector<uint16_t> data(pixelCount * 4);

	for (uint32_t pixel = 0; pixel < pixelCount; pixel++) {
		uint16_t channels[4];
		channels[3] = FLOAT16_ONE;

		int idx = 0;
		for (int i = image.num_channels - 1; i >= 0; i -= 1) {
			// channels are in reverse order
			float value = pData[i][pixel];
			channels[idx] = _floatToHalf(value);

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

	size_t byteSize = data.size() * sizeof(uint16_t);
	std::vector<uint8_t> bytes(byteSize);
	memcpy(bytes.data(), data.data(), byteSize);

	return new Image(width, height, 1, Image::Format::RGBAF16, bytes);
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
