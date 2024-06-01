#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_multiview : enable

#include "include/cubemap_incl.glsl"
#include "include/filter_incl.glsl"

layout(location = 0) in vec2 inCoords;
layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform samplerCube cubeSampler;

layout(push_constant) uniform PreFilterPushConstants {
	uint size;
	float roughness;
};

float distributionGGX(float nDotH, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float nDotH2 = nDotH * nDotH;

	float nom = a2;
	float denom = (nDotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

void main() {
	vec3 n = mapToCube(inCoords, gl_ViewIndex, true);
	vec3 r = n;
	vec3 v = r;

	vec3 filteredColor = vec3(0.0);
	float totalWeight = 0.0;

	const uint SAMPLE_COUNT = 2048u;

	for (uint i = 0u; i < SAMPLE_COUNT; i++) {
		// generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
		vec2 xi = hammersley(i, SAMPLE_COUNT);
		vec3 h = importanceSampleGGX(xi, n, roughness);
		vec3 l = normalize(2.0 * dot(v, h) * h - v);

		float nDotL = max(dot(n, l), 0.0);

		if (nDotL > 0.0) {
			float nDotH = max(dot(n, h), 0.0);
			float hDotV = max(dot(h, v), 0.0);

			// sample from the environment's mip level based on roughness/pdf
			float d = distributionGGX(nDotH, roughness);
			float pdf = d * nDotH / (4.0 * hDotV) + 0.0001;

			float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
			float saTexel = 4.0 * PI / (6.0 * float(size) * float(size));

			float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

			filteredColor += textureLod(cubeSampler, l, mipLevel).rgb * nDotL;
			totalWeight += nDotL;
		}
	}

	filteredColor = filteredColor / totalWeight;
	outFragColor = vec4(filteredColor, 1.0);
}
