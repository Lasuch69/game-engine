#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
	vec3 viewPosition;

	int directionalLightCount;
	int pointLightCount;
};

struct DirectionalLight {
	vec3 direction;
	float intensity;
	vec3 color;
	float _padding;
};

layout(set = 1, binding = 0) readonly buffer DirectionalLightBuffer {
	DirectionalLight directionalLights[];
};

struct PointLight {
	vec3 position;
	float range;
	vec3 color;
	float intensity;
};

layout(set = 1, binding = 1) readonly buffer PointLightBuffer {
	PointLight pointLights[];
};

layout(set = 2, binding = 0) uniform sampler2D albedoTexture;
layout(set = 2, binding = 1) uniform sampler2D normalTexture;
layout(set = 2, binding = 2) uniform sampler2D metallicTexture;
layout(set = 2, binding = 3) uniform sampler2D roughnessTexture;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;

layout(location = 4) in vec3 inBitangent;

layout(location = 0) out vec4 fragColor;

layout(early_fragment_tests) in;

const float PI = 3.14159265359;

float saturate(float n) {
	return clamp(n, 0.0, 1.0);
}

vec3 deriveNormalZ(vec2 n) {
	float z = sqrt(1.0 - saturate(dot(n, n)));
	return vec3(n.x, n.y, z);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = geometrySchlickGGX(NdotV, roughness);
	float ggx1 = geometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 sRGBToLinear(vec3 c) {
	return pow(c, vec3(2.2));
}

void main() {
	vec3 albedo = sRGBToLinear(texture(albedoTexture, inUV).rgb);
	vec3 normal = texture(normalTexture, inUV).xyz;
	float metallic = texture(metallicTexture, inUV).r;
	float roughness = texture(roughnessTexture, inUV).r;

	// map from [0, 1] to [-1, +1] range
	normal = normal * 2.0 - vec3(1.0);
	normal = deriveNormalZ(normal.xy);

	// tangent space to world space
	normal = mat3(inTangent, inBitangent, inNormal) * normal;

	vec3 N = normalize(normal);
	vec3 V = normalize(viewPosition - inPosition);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = vec3(0.0);

	// directional lights
	for (int i = 0; i < directionalLightCount; i++) {
		vec3 lightDir = directionalLights[i].direction;
		vec3 lightColor = directionalLights[i].color * directionalLights[i].intensity;

		vec3 L = normalize(-lightDir);
		vec3 H = normalize(V + L);

		// cook-torrance brdf
		float NDF = distributionGGX(N, H, roughness);
		float G = geometrySmith(N, V, L, roughness);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = numerator / denominator;

		// add to outgoing radiance Lo
		float NdotL = max(dot(N, L), 0.0);
		Lo += (kD * albedo / PI + specular) * NdotL;
	}

	// point lights
	for (int i = 0; i < pointLightCount; i++) {
		vec3 lightPos = pointLights[i].position;
		vec3 lightColor = pointLights[i].color * pointLights[i].intensity;

		// calculate per-light radiance
		vec3 L = normalize(lightPos - inPosition);
		vec3 H = normalize(V + L);
		float distance = length(lightPos - inPosition);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColor * attenuation;

		// cook-torrance brdf
		float NDF = distributionGGX(N, H, roughness);
		float G = geometrySmith(N, V, L, roughness);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
		vec3 specular = numerator / denominator;

		// add to outgoing radiance Lo
		float NdotL = max(dot(N, L), 0.0);
		Lo += (kD * albedo / PI + specular) * radiance * NdotL;
	}

	vec3 ambient = vec3(0.03) * albedo;
	vec3 color = ambient + Lo;

	fragColor = vec4(color, 1.0);
}
