#include <cmath>
#include <cstdint>
#include <cstring>

#include <SDL3/SDL_log.h>

#include "light_storage.h"

// in lumens
const float INTENSITY_THRESHOLD = 1.0;

// 1 / (PI * 4)
const float FACTOR = 0.07958;

float _calculateRange(float intensity) {
	return sqrtf(intensity * FACTOR / INTENSITY_THRESHOLD);
}

ObjectID LightStorage::lightCreate(LightType type) {
	LightRD light;
	light.type = type;

	return _lightOwner.append(light);
}

void LightStorage::lightSetTransform(ObjectID lightID, const float *pTransform) {
	if (!_lightOwner.has(lightID)) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Light: %ld is invalid!", lightID);
		return;
	}

	memcpy(_lightOwner[lightID].transform, pTransform, sizeof(float) * 16);
}

void LightStorage::lightSetColor(ObjectID lightID, const float *pColor) {
	if (!_lightOwner.has(lightID)) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Light: %ld is invalid!", lightID);
		return;
	}

	memcpy(_lightOwner[lightID].color, pColor, sizeof(float) * 3);
}

void LightStorage::lightSetIntensity(ObjectID lightID, float intensity) {
	if (!_lightOwner.has(lightID)) {
		SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Light: %ld is invalid!", lightID);
		return;
	}

	_lightOwner[lightID].intensity = intensity;
	_lightOwner[lightID].range = _calculateRange(intensity);
}

void LightStorage::lightFree(ObjectID light) {
	_lightOwner.remove(light);
}

uint32_t LightStorage::getDirectionalLightCount() {
	const std::unordered_map<ObjectID, LightRD> &map = _lightOwner.map();

	uint32_t count = 0;
	for (auto it = map.begin(); it != map.end(); ++it)
		if (it->second.type == LightType::Directional)
			count++;

	return count;
}

uint32_t LightStorage::getPointLightCount() {
	const std::unordered_map<ObjectID, LightRD> &map = _lightOwner.map();

	uint32_t count = 0;
	for (auto it = map.begin(); it != map.end(); ++it)
		if (it->second.type == LightType::Point)
			count++;

	return count;
}
