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

const vec3 ambient = vec3(0.025);
const vec3 specColor = vec3(1.0, 1.0, 1.0);
const float shininess = 16.0;

void main() {
	vec3 albedo = texture(albedoTexture, inTexCoord).rgb;
	vec3 finalColor = albedo * ambient;

	vec3 viewDir = normalize(ubo.viewPosition - inPosition);

	for (int i = 0; i < ubo.lightCount; i++) {
		vec3 lightPos = lights.data[i].position;
		vec3 lightColor = lights.data[i].color;
		float intensity = lights.data[i].intensity;

		vec3 lightDir = lightPos - inPosition;
		float distance = length(lightDir);
		distance = distance * distance;
		lightDir = normalize(lightDir);

		float lambertian = max(dot(lightDir, inNormal), 0.0);
		float specular = 0.0;

		if (lambertian > 0.0) {
			// this is blinn phong
			vec3 halfDir = normalize(lightDir + viewDir);
			float specAngle = max(dot(halfDir, inNormal), 0.0);
			specular = pow(specAngle, shininess);
		}

		vec3 light = lambertian * lightColor * intensity / distance +
			specColor * specular * lightColor * intensity / distance;

		finalColor += albedo * light;
	}

	fragColor = vec4(finalColor, 1.0);
}
