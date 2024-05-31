#ifndef LIGHT_STORAGE_H
#define LIGHT_STORAGE_H

#include <cstdint>

#include "../object_owner.h"

const uint32_t MAX_DIRECTIONAL_LIGHT_COUNT = 8;
const uint32_t MAX_POINT_LIGHT_COUNT = 2048;

enum class LightType {
	Directional,
	Point,
};

class LightStorage {
public:
	static LightStorage &getSingleton() {
		static LightStorage instance;
		return instance;
	}

private:
	LightStorage() {}

	typedef struct {
		float direction[3];
		float _padding;

		float color[3];
		float intensity;
	} DirectionalData;
	static_assert(sizeof(DirectionalData) % 16 == 0, "DirectionalData is not multiple of 16");

	typedef struct {
		float position[3];
		float range;

		float color[3];
		float intensity;
	} PunctualData;
	static_assert(sizeof(PunctualData) % 16 == 0, "PunctualData is not multiple of 16");

	typedef struct {
		LightType type;

		float transform[16];
		float range;

		float color[3];
		float intensity;
	} LightRD;

	ObjectOwner<LightRD> _lights;

public:
	LightStorage(LightStorage const &) = delete;
	void operator=(LightStorage const &) = delete;

	ObjectID lightCreate(LightType type);
	void lightSetTransform(ObjectID light, const float *pTransform);
	void lightSetRange(ObjectID light, float range);
	void lightSetColor(ObjectID light, const float *pColor);
	void lightSetIntensity(ObjectID light, float intensity);
	void lightFree(ObjectID light);

	uint32_t getDirectionalLightCount() const;
	uint32_t getPointLightCount() const;

	size_t getDirectionalLightSSBOSize() const;
	size_t getPointLightSSBOSize() const;

	uint8_t *getDirectionalLightBuffer() const;
	uint8_t *getPointLightBuffer() const;
};

typedef LightStorage LS;

#endif // !LIGHT_STORAGE_H
