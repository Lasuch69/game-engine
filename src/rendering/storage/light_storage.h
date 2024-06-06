#ifndef LIGHT_STORAGE_H
#define LIGHT_STORAGE_H

#include <cstdint>

#include <rendering/object_owner.h>

const uint32_t MAX_DIRECTIONAL_LIGHT_COUNT = 8;
const uint32_t MAX_POINT_LIGHT_COUNT = 2048;

enum class LightType {
	Directional,
	Point,
};

class LightStorage {
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

	ObjectOwner<LightRD> _lightOwner;

public:
	ObjectID lightCreate(LightType type);
	void lightSetTransform(ObjectID lightID, const float *pTransform);
	void lightSetColor(ObjectID lightID, const float *pColor);
	void lightSetIntensity(ObjectID lightID, float intensity);
	void lightFree(ObjectID lightID);

	uint32_t getDirectionalLightCount();
	uint32_t getPointLightCount();
};

#endif // !LIGHT_STORAGE_H
