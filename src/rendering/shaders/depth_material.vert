#version 450

layout(push_constant) uniform MaterialPushConstants {
	mat4 projView;
	mat4 model;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;

void main() {
	gl_Position = projView * model * vec4(inPosition, 1.0);
}
