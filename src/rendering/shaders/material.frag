#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 projView;
	vec3 viewPosition;
	int lightCount;
} ubo;

struct LightData {
	vec3 position;
	float range;
	vec3 color;
	float intensity;
};

layout(set = 1, binding = 0) readonly buffer LightDataBuffer {
	LightData data[];
} lights;

layout(set = 2, binding = 0) uniform sampler2D albedoTexture;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;

const float PI = 3.14159265359;

const float roughness = 0.5f;
const float metallic = 0.0f;

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

void main() {
	vec3 albedo = texture(albedoTexture, inTexCoord).rgb;

	vec3 N = normalize(inNormal);
	vec3 V = normalize(ubo.viewPosition - inPosition);

	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	vec3 Lo = vec3(0.0);
	for (int i = 0; i < ubo.lightCount; i++) {
		vec3 lightPos = lights.data[i].position;
		vec3 lightColor = lights.data[i].color;
		float intensity = lights.data[i].intensity;

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

	color = pow(color, vec3(1.0/2.2));

	fragColor = vec4(color, 1.0);
}
