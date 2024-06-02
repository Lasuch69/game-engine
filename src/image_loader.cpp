#include <cassert>
#include <cstdint>
#include <cstring>

#include <ConvectionKernels.h>
#include <SDL2/SDL_log.h>
#include <stb/stb_image.h>
#include <tinyexr/tinyexr.h>

#include "rendering/rendering_device.h"

#include "image.h"
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

_Float16 *getPixel(uint32_t x, uint32_t y, uint32_t w, uint32_t h, _Float16 *pData) {
	if (x >= w)
		x = w - 1;

	if (y >= h)
		y = h - 1;

	size_t offset = ((y * w) + x) * 4;
	return &pData[offset];
}

cvtt::PixelBlockF16 readBlock(
		uint32_t offsetX, uint32_t offsetY, uint32_t w, uint32_t h, _Float16 *pBytes) {
	cvtt::PixelBlockF16 block;

	uint32_t i = 0;
	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			uint32_t _x = offsetX + x;
			uint32_t _y = offsetY + y;

			uint16_t *pPixel = reinterpret_cast<uint16_t *>(getPixel(_x, _y, w, h, pBytes));

			block.m_pixels[i][0] = pPixel[0];
			block.m_pixels[i][1] = pPixel[1];
			block.m_pixels[i][2] = pPixel[2];
			block.m_pixels[i][3] = pPixel[3];

			i++;
		}
	}

	SDL_LogInfo(0, "x: %d, y: %d", offsetX, offsetY);
	return block;
}

void ImageLoader::_compressCVTT(const Image *pImage) {
	cvtt::Options options;
	options.flags = cvtt::Flags::Ultra;

	uint32_t width = pImage->getWidth();
	uint32_t height = pImage->getHeight();

	size_t size = width * height * 4;

	// FIXME: _Float16 is probably GCC only :>
	std::vector<_Float16> data = {};
	data.resize(size);

	{
		std::vector<uint8_t> bytes = pImage->getData();
		float *pData = reinterpret_cast<float *>(bytes.data());

		for (size_t i = 0; i < size; i++) {
			// cast float to half float
			data[i] = static_cast<_Float16>(pData[i]);
		}
	}

	const uint32_t BLOCK_SIZE = 4;

	uint32_t w = ((width + 3) / 4) * 4;
	uint32_t h = ((height + 3) / 4) * 4;

	uint8_t *pOutBytes = new uint8_t[w * h];
	size_t byteOffset = 0;

	for (int by = 0; by < height; by += BLOCK_SIZE) {
		for (int bx = 0; bx < width; bx += BLOCK_SIZE * cvtt::NumParallelBlocks) {
			cvtt::PixelBlockF16 pInputBlocks[cvtt::NumParallelBlocks];

			for (int i = 0; i < 8; i++)
				pInputBlocks[i] = readBlock(bx + i * BLOCK_SIZE, by, width, height, data.data());

			const size_t BYTE_SIZE = 16 * cvtt::NumParallelBlocks;

			uint8_t pOutputBlocks[BYTE_SIZE];
			cvtt::Kernels::EncodeBC6HS(pOutputBlocks, pInputBlocks, options);

			memcpy(&pOutBytes[byteOffset], pOutputBlocks, BYTE_SIZE);
			byteOffset += BYTE_SIZE;
		}
	}

	{
		RD &rd = RD::getSingleton();

		AllocatedImage image = rd.imageCreate(width, height, vk::Format::eBc6HSfloatBlock, 1,
				vk::ImageUsageFlagBits::eTransferDst);

		rd.imageLayoutTransition(image.image, vk::Format::eBc6HSfloatBlock, 1, 1,
				vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

		uint32_t size = w * h;
		rd.imageSend(
				image.image, width, height, pOutBytes, size, vk::ImageLayout::eTransferDstOptimal);
	}
}

Image *ImageLoader::loadFromFile(const std::filesystem::path &path) {
	return _loadFromFile(path);
}

Image *ImageLoader::loadFromMemory(const std::vector<uint8_t> &bytes) {
	return _loadFromMemory(bytes);
}

Image *ImageLoader::loadHDRFromFile(const std::filesystem::path &path) {
	Image *pImage = _loadHDRFromFile(path);

	ImageLoader loader;
	loader._compressCVTT(pImage);

	return pImage;
}

Image *ImageLoader::loadEXRFromFile(const std::filesystem::path &path) {
	return _loadEXRFromFile(path);
}
