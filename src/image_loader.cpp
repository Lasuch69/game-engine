#include <cassert>
#include <cstdint>

#include <SDL3/SDL_log.h>

#include <stb/stb_image.h>
#include <tinyexr/tinyexr.h>

#include "image_loader.h"

Image::Format asFormat(uint32_t channels) {
	switch (channels) {
		case 1:
			return Image::Format::R8;
		case 2:
			return Image::Format::RG8;
		case 3:
			return Image::Format::RGB8;
		case 4:
			return Image::Format::RGBA8;
	}

	return Image::Format::R8;
}

Image *_loadFromFile(const std::filesystem::path &path) {
	int width, height, channels;
	stbi_uc *pData = stbi_load(path.c_str(), &width, &height, &channels, STBI_default);

	if (pData == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load image from path \'%s\'", path.c_str());
		return nullptr;
	}

	size_t size = width * height * channels;

	std::vector<uint8_t> data(size);
	memcpy(data.data(), pData, size);
	stbi_image_free(pData);

	return new Image(width, height, asFormat(channels), data);
}

Image *_loadFromMemory(const std::vector<uint8_t> &bytes) {
	int width, height, channels;
	stbi_uc *pData = stbi_load_from_memory(
			bytes.data(), bytes.size(), &width, &height, &channels, STBI_default);

	if (pData == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load image from memory");
		return nullptr;
	}

	size_t size = width * height * channels;

	std::vector<uint8_t> data(size);
	memcpy(data.data(), pData, size);
	stbi_image_free(pData);

	return new Image(width, height, asFormat(channels), data);
}

bool _parseEXRVersionFromFile(EXRVersion *pVersion, const std::filesystem::path &path) {
	EXRVersion version;
	int err = ParseEXRVersionFromFile(&version, path.c_str());

	if (err != TINYEXR_SUCCESS)
		return false;

	// multipart EXR is unsupported
	if (version.multipart)
		return false;

	*pVersion = version;
	return true;
}

bool _parseEXRHeaderFromFile(
		EXRHeader *pHeader, const EXRVersion *pVersion, const std::filesystem::path &path) {
	EXRHeader header;
	InitEXRHeader(&header);

	const char *errMsg = nullptr;
	int err = ParseEXRHeaderFromFile(&header, pVersion, path.c_str(), &errMsg);

	if (err != TINYEXR_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", errMsg);
		FreeEXRErrorMessage(errMsg);
		return false;
	}

	*pHeader = header;
	return true;
}

Image *_loadEXRFromFile(const std::filesystem::path &path) {
	EXRVersion version;
	if (_parseEXRVersionFromFile(&version, path) == false)
		return nullptr;

	EXRHeader header;
	if (_parseEXRHeaderFromFile(&header, &version, path) == false)
		return nullptr;

	// read 16-bit float as 32-bit
	for (int i = 0; i < header.num_channels; i++) {
		if (header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
			header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
		}
	}

	EXRImage image;
	InitEXRImage(&image);

	const char *errMsg = nullptr;
	int err = LoadEXRImageFromFile(&image, &header, path.c_str(), &errMsg);

	if (err != TINYEXR_SUCCESS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "%s", errMsg);
		FreeEXRHeader(&header);
		FreeEXRErrorMessage(errMsg);
		return nullptr;
	}

	// force RGBA format limit
	const int MAX_CHANNELS = 4;

	if (image.num_channels > MAX_CHANNELS) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR,
				"EXR Image %s, has %d number of channels. Max supported channel count is %d.",
				path.c_str(), image.num_channels, MAX_CHANNELS);

		FreeEXRImage(&image);
		FreeEXRHeader(&header);
		return nullptr;
	}

	int width = image.width;
	int height = image.height;
	int numChannels = image.num_channels;

	size_t pixelCount = width * height;
	std::vector<float> pixels(pixelCount * numChannels);

	if (image.images != nullptr) {
		float **pPixels = reinterpret_cast<float **>(image.images);

		for (size_t pixel = 0; pixel < pixelCount; pixel++) {
			for (int channel = 0; channel < numChannels; channel++) {
				size_t offset = (pixel * numChannels) + channel;

				// src ABGR -> dst RGBA
				int c = (numChannels - 1) - channel;
				float value = pPixels[c][pixel];

				pixels[offset] = value;
			}
		}

	} else {
		FreeEXRImage(&image);
		FreeEXRHeader(&header);
		return nullptr;
	}

	FreeEXRImage(&image);
	FreeEXRHeader(&header);

	size_t byteSize = pixels.size() * sizeof(float);

	std::vector<uint8_t> bytes(byteSize);
	memcpy(bytes.data(), pixels.data(), byteSize);

	return new Image(width, height, Image::Format::RGBA32F, bytes);
}

Image *_loadHDRFromFile(const std::filesystem::path &path) {
	if (!stbi_is_hdr(path.c_str()))
		return nullptr;

	int width, height, channels;
	float *pData = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_default);

	if (pData == nullptr) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not load image from path \'%s\'", path.c_str());
		return nullptr;
	}

	// TODO: probably handle other formats
	assert(channels == 3);

	// convert RGB to RGBA
	const int CHANNELS = 4;
	const float ALPHA = 1.0f;

	size_t size = width * height * CHANNELS;
	std::vector<float> data(size);

	size_t srcIdx = 0;
	for (size_t i = 0; i < size; i++) {
		// source has 3 channels
		// every 4th value is alpha
		if ((i + 1) % 4 == 0) {
			data[i] = ALPHA;
			continue;
		}

		data[i] = pData[srcIdx];
		srcIdx++;
	}

	stbi_image_free(pData);

	size_t byteSize = size * sizeof(float);
	std::vector<uint8_t> bytes(byteSize);
	memcpy(bytes.data(), data.data(), byteSize);

	return new Image(width, height, Image::Format::RGBA32F, bytes);
}

Image *ImageLoader::loadFromFile(const std::filesystem::path &path) {
	return _loadFromFile(path);
}

Image *ImageLoader::loadFromMemory(const std::vector<uint8_t> &bytes) {
	return _loadFromMemory(bytes);
}

Image *ImageLoader::loadHDRFromFile(const std::filesystem::path &path) {
	return _loadHDRFromFile(path);
}

Image *ImageLoader::loadEXRFromFile(const std::filesystem::path &path) {
	return _loadEXRFromFile(path);
}
