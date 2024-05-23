#version 450

#extension GL_EXT_multiview : enable

#include "include/cubemap_incl.glsl"
#include "include/filter_incl.glsl"

layout(push_constant) uniform PreFilterPushConstants {
	uint srcCubeSize;
	float roughness;
};

layout(location = 0) in vec2 coords;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform samplerCube cubeSampler;

float distributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

void main() {
	vec3 N = mapToCube(coords, gl_ViewIndex, true);
	vec3 R = N;
	vec3 V = R;

	const uint SAMPLE_COUNT = 2048u;
	vec3 prefilteredColor = vec3(0.0);
	float totalWeight = 0.0;

	for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
		// generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
		vec2 Xi = hammersley(i, SAMPLE_COUNT);
		vec3 H = importanceSampleGGX(Xi, N, roughness);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float NdotL = max(dot(N, L), 0.0);
		if (NdotL > 0.0) {
			// sample from the environment's mip level based on roughness/pdf
			float D = distributionGGX(N, H, roughness);
			float NdotH = max(dot(N, H), 0.0);
			float HdotV = max(dot(H, V), 0.0);
			float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

			float saTexel = 4.0 * PI / (6.0 * float(srcCubeSize) * float(srcCubeSize));
			float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

			float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

			prefilteredColor += textureLod(cubeSampler, L, mipLevel).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	prefilteredColor = prefilteredColor / totalWeight;
	fragColor = vec4(prefilteredColor, 1.0);
}
