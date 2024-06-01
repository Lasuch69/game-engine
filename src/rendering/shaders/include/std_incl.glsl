const float PI = 3.14159265359;

float saturate(float v) {
	return clamp(v, 0.0, 1.0);
}

vec3 sRGBToLinear(vec3 c) {
	return pow(c, vec3(2.2));
}

vec3 unpackNormal(vec2 rg, mat3 tbn) {
	rg = rg * 2.0 - vec2(1.0);
	float b = sqrt(1.0 - saturate(dot(rg, rg)));

	vec3 n = vec3(rg, b);
	return normalize(tbn * n);
}
