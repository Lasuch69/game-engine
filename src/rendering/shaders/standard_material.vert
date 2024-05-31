#version 450

layout(push_constant) uniform MeshPushConstants {
	mat4 projView;
	mat4 model;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 outPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec2 outUV;

layout(location = 4) out vec3 outBitangent;

void main() {
	vec4 vertPos4 = model * vec4(inPosition, 1.0);

	vec3 T = normalize(vec3(model * vec4(inTangent, 0.0)));
	vec3 N = normalize(vec3(model * vec4(inNormal, 0.0)));

	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);

	// then retrieve perpendicular vector B with the cross product of T and N
	vec3 B = cross(N, T);

	outPosition = vec3(vertPos4) / vertPos4.w;
	outNormal = N;
	outTangent = T;
	outUV = inUV;

	outBitangent = B;

	gl_Position = projView * model * vec4(inPosition, 1.0);
}
