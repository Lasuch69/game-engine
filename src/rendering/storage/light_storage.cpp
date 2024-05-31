#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "light_storage.h"

#define CHECK_IF_VALID(owner, id, what)                                                            \
	if (!owner.has(id)) {                                                                          \
		std::cout << "ERROR: " << what << ": " << id << " is not valid resource!" << std::endl;    \
		return;                                                                                    \
	}

ObjectID LS::lightCreate(LightType type) {
	LightRD light;
	light.type = type;

	return _lights.insert(light);
}

void LS::lightSetTransform(ObjectID light, const float *pTransform) {
	CHECK_IF_VALID(_lights, light, "Light");
	memcpy(_lights[light].transform, pTransform, sizeof(float) * 16);
}

void LS::lightSetRange(ObjectID light, float range) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].range = range;
}

void LS::lightSetColor(ObjectID light, const float *pColor) {
	CHECK_IF_VALID(_lights, light, "Light");
	memcpy(_lights[light].color, pColor, sizeof(float) * 3);
}

void LS::lightSetIntensity(ObjectID light, float intensity) {
	CHECK_IF_VALID(_lights, light, "Light");
	_lights[light].intensity = intensity;
}

void LS::lightFree(ObjectID light) {
	_lights.free(light);
}

uint32_t LS::getDirectionalLightCount() const {
	uint32_t count = 0;
	for (const auto &[_, light] : _lights.map()) {
		if (light.type != LightType::Directional)
			continue;

		count++;
	}

	return count;
}

uint32_t LS::getPointLightCount() const {
	uint32_t count = 0;
	for (const auto &[_, light] : _lights.map()) {
		if (light.type != LightType::Point)
			continue;

		count++;
	}

	return count;
}

size_t LS::getDirectionalLightSSBOSize() const {
	return sizeof(DirectionalData) * MAX_DIRECTIONAL_LIGHT_COUNT;
}

size_t LS::getPointLightSSBOSize() const {
	return sizeof(PunctualData) * MAX_POINT_LIGHT_COUNT;
}

uint8_t *LS::getDirectionalLightBuffer() const {
	size_t size = MAX_DIRECTIONAL_LIGHT_COUNT;
	DirectionalData *pBuffer =
			reinterpret_cast<DirectionalData *>(calloc(size, sizeof(DirectionalData)));

	size_t idx = 0;
	for (const auto &[_, _light] : _lights.map()) {
		if (idx >= size)
			break;

		if (_light.type != LightType::Directional)
			continue;

		float *t = (float *)_light.transform;
		float direction[3];
		direction[2] = -t[8] + -t[9] + -t[10];

		DirectionalData data;
		memcpy(data.direction, &direction, sizeof(data.direction));
		memcpy(data.color, &_light.color, sizeof(data.color));
		data.intensity = _light.intensity;

		pBuffer[idx] = data;
		idx++;
	}

	return reinterpret_cast<uint8_t *>(pBuffer);
}

uint8_t *LS::getPointLightBuffer() const {
	size_t size = MAX_POINT_LIGHT_COUNT;
	PunctualData *pBuffer = reinterpret_cast<PunctualData *>(calloc(size, sizeof(PunctualData)));

	size_t idx = 0;
	for (const auto &[_, _light] : _lights.map()) {
		if (idx >= size)
			break;

		if (_light.type != LightType::Point)
			continue;

		float *t = (float *)_light.transform;
		float position[3] = { t[11], t[12], t[13] };

		PunctualData data;
		memcpy(data.position, &position, sizeof(data.position));
		data.range = _light.range;
		memcpy(data.color, &_light.color, sizeof(data.color));
		data.intensity = _light.intensity;

		pBuffer[idx] = data;
		idx++;
	}

	return reinterpret_cast<uint8_t *>(pBuffer);
}
