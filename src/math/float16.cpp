#include "float16.h"

half floatToHalf(float value) {
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

float halfToFloat(half value) {
	uint32_t sign = (value >> 15) & 0x1;
	uint32_t exp = (value >> 10) & 0x1f;
	uint32_t frac = value & 0x3ff;

	exp = exp + (127 - 15);

	union {
		float value;
		uint32_t bits;
	} data;

	data.bits = (sign << 31) + (exp << 23) + (frac << 13);
	return data.value;
}

// does not support negative numbers
half halfAverageApprox(half a, half b) {
	return (a + b) >> 1;
}
