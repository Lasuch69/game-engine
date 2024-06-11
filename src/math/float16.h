#ifndef FLOAT16_H
#define FLOAT16_H

#include <cstdint>

#define FLOAT16_MAX 0x7bff // 01111011 11111111 =  65504.0
#define FLOAT16_MIN 0xfbff // 11111011 11111111 = -65504.0
#define FLOAT16_ONE 0x3c00 // 00111100 00000000 =  1.0

typedef uint16_t half;

half floatToHalf(float value);
float halfToFloat(half value);

half halfAverageApprox(half a, half b);

#endif // !FLOAT16_H
