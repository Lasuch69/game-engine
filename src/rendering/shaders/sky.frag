#version 450

layout(location = 0) in vec4 inClipPosition;
layout(location = 0) out vec4 outFragColor;

layout(binding = 0) uniform samplerCube environmentSampler;

layout(push_constant) uniform SkyConstants {
	mat4 invProj;
	mat4 invView;
};

void main() {
	vec4 viewPos = invProj * inClipPosition;
	vec3 viewRayDir = viewPos.xyz / viewPos.w;
	vec3 rayDir = normalize((invView * vec4(viewRayDir, 0.0)).xyz);

	outFragColor = texture(environmentSampler, rayDir);
}
