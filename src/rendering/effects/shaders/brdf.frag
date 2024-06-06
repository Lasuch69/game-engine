#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/filter_incl.glsl"

layout(location = 0) in vec2 inCoords;
layout(location = 0) out vec2 outBRDF;

const uint SAMPLE_COUNT = 1024u;

float saturate(float v) {
	return clamp(v, 0.0, 1.0);
}

float geometrySchlickGGX(float nDotV, float roughness) {
	float a = roughness;
	float k = (a * a) / 2.0;

	float nom = nDotV;
	float denom = nDotV * (1.0 - k) + k;
	return nom / denom;
}

float geometrySmith(float nDotV, float nDotL, float roughness) {
	float ggx2 = geometrySchlickGGX(nDotV, roughness);
	float ggx1 = geometrySchlickGGX(nDotL, roughness);
	return ggx1 * ggx2;
}

vec2 integrateBRDF(float nDotV, float roughness) {
	vec3 v;
	v.x = sqrt(1.0 - nDotV * nDotV);
	v.y = 0.0;
	v.z = nDotV;

	float a = 0.0;
	float b = 0.0;

	vec3 n = vec3(0.0, 0.0, 1.0);

	for (uint i = 0u; i < SAMPLE_COUNT; i++) {
		vec2 xi = hammersley(i, SAMPLE_COUNT);
		vec3 h = importanceSampleGGX(xi, n, roughness);
		vec3 l = 2.0 * dot(v, h) * h - v;

		float nDotL = saturate(l.z);
		float nDotH = saturate(h.z);
		float vDotH = saturate(dot(v, h));

		if (nDotL > 0.0) {
			float geometry = geometrySmith(nDotV, nDotL, roughness);
			float geometryVis = (geometry * vDotH) / (nDotH * nDotV);
			float fc = pow(1.0 - vDotH, 5.0);

			a += (1.0 - fc) * geometryVis;
			b += fc * geometryVis;
		}
	}

	return vec2(a, b) / float(SAMPLE_COUNT);
}

void main() {
	outBRDF = integrateBRDF(inCoords.x, inCoords.y);
}
