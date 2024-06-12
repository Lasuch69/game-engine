#ifndef MATH_UTIL_H
#define MATH_UTIL_H

template <typename T> T max(T a, T b) {
	return (a > b) ? a : b;
}

template <typename T> T min(T a, T b) {
	return (a < b) ? a : b;
}

// min(max(value, min), max)
template <typename T> T clamp(T value, T min, T max) {
	T v = (value > min) ? value : min;
	return (v < max) ? value : max;
}

#endif // !MATH_UTIL_H
