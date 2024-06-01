#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/light_incl.glsl"
#include "include/std_incl.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform UniformBufferObject {
	vec3 viewPosition;

	int directionalLightCount;
	int pointLightCount;
};

layout(set = 1, binding = 0) uniform samplerCube irradianceSampler;
layout(set = 1, binding = 1) uniform samplerCube specularSampler;
layout(set = 1, binding = 2) uniform sampler2D lutSampler;

layout(set = 2, binding = 0) readonly buffer DirectionalLightSSBO {
	DirectionalLight directionalLights[];
};

layout(set = 2, binding = 1) readonly buffer PointLightSSBO {
	PointLight pointLights[];
};

layout(set = 3, binding = 0) uniform sampler2D albedoSampler;
layout(set = 3, binding = 1) uniform sampler2D normalSampler;
layout(set = 3, binding = 2) uniform sampler2D metallicSampler;
layout(set = 3, binding = 3) uniform sampler2D roughnessSampler;

layout(early_fragment_tests) in;

float distributionGGX(float nDotH, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float nDotH2 = nDotH * nDotH;

	float num = a2;
	float denom = (nDotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;
	return num / denom;
}

float geometrySchlickGGX(float nDotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = nDotV;
	float denom = nDotV * (1.0 - k) + k;
	return num / denom;
}

float geometrySmith(float nDotV, float nDotL, float roughness) {
	float ggx2 = geometrySchlickGGX(nDotV, roughness);
	float ggx1 = geometrySchlickGGX(nDotL, roughness);
	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 f0) {
	return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 f0, float roughness) {
	return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(saturate(1.0 - cosTheta), 5.0);
}

vec3 cookTorranceBRDF(float nDotV, float nDotL, float nDotH, float cosTheta, vec3 f0, float roughness, float metallic, vec3 albedo, vec3 radiance) {
	float distribution = distributionGGX(nDotH, roughness);
	float geometrySmith = geometrySmith(nDotV, nDotL, roughness);
	vec3 fresnel = fresnelSchlick(cosTheta, f0);

	vec3 kS = fresnel;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	vec3 numerator = distribution * geometrySmith * fresnel;
	float denominator = 4.0 * nDotV * nDotL + 0.0001;
	vec3 specular = numerator / denominator;

	return (kD * albedo / PI + specular) * radiance * nDotL;
}

void main() {
	vec3 albedo = sRGBToLinear(texture(albedoSampler, inUV).rgb);
	float metallic = texture(metallicSampler, inUV).r;
	float roughness = texture(roughnessSampler, inUV).r;

	mat3 tbn = mat3(inTangent, inBitangent, inNormal);
	vec3 normal = unpackNormal(texture(normalSampler, inUV).rg, tbn);
	vec3 view = normalize(viewPosition - inPosition);

	float nDotV = max(dot(normal, view), 0.0);

	vec3 f0 = vec3(0.04);
	f0 = mix(f0, albedo, metallic);

	vec3 lightValue = vec3(0.0);

	for (int i = 0; i < directionalLightCount; i++) {
		DirectionalLight light = directionalLights[i];

		vec3 lightDirection = normalize(-light.direction);
		vec3 halfVector = normalize(view + lightDirection);

		float nDotL = max(dot(normal, lightDirection), 0.0);
		float nDotH = max(dot(normal, halfVector), 0.0);
		float cosTheta = max(dot(halfVector, view), 0.0);

		vec3 radiance = light.color * light.intensity;

		lightValue += cookTorranceBRDF(nDotV, nDotL, nDotH, cosTheta, f0, roughness, metallic, albedo, radiance);
	}

	for (int i = 0; i < pointLightCount; i++) {
		PointLight light = pointLights[i];

		vec3 lightDirection = normalize(light.position - inPosition);
		vec3 halfVector = normalize(view + lightDirection);

		float nDotL = max(dot(normal, lightDirection), 0.0);
		float nDotH = max(dot(normal, halfVector), 0.0);
		float cosTheta = max(dot(halfVector, view), 0.0);

		float distance = length(light.position - inPosition);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = (light.color * light.intensity) * attenuation;

		lightValue += cookTorranceBRDF(nDotV, nDotL, nDotH, cosTheta, f0, roughness, metallic, albedo, radiance);
	}

	vec3 fresnel = fresnelSchlickRoughness(nDotV, f0, roughness);

	vec3 kS = fresnel;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metallic;

	vec3 irradiance = texture(irradianceSampler, normal).rgb;
	vec3 diffuse = irradiance * albedo;

	const float MAX_REFLECTION_LOD = 4.0;
	float lod = roughness * MAX_REFLECTION_LOD;

	vec3 reflect = 2.0 * dot(view, normal) * normal - view;
	vec3 filteredColor = textureLod(specularSampler, reflect, lod).rgb;
	vec2 brdf = texture(lutSampler, vec2(nDotV, roughness)).rg;
	vec3 specular = filteredColor * (fresnel * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular);
	vec3 color = ambient + lightValue;

	outFragColor = vec4(color, 1.0);
}
