#version 450

layout(push_constant) uniform SkyConstants {
	mat4 invProj;
	mat4 invView;
};

layout(binding = 0) uniform samplerCube environment;

layout(location = 0) in vec4 clipPosition;

layout(location = 0) out vec4 fragColor;

void main() {
	vec4 viewPos = invProj * clipPosition;
	vec3 viewRayDir = viewPos.xyz / viewPos.w;
	vec3 rayDir = normalize((invView * vec4(viewRayDir, 0.0)).xyz);

	fragColor = texture(environment, rayDir);
}
