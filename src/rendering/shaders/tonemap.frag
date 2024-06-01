#version 450

#extension GL_GOOGLE_include_directive : enable

#include "include/tonemap_incl.glsl"

layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0, input_attachment_index = 0) uniform subpassInput inputColor;

layout(push_constant) uniform TonemapParameterConstants {
	float exposure;
	float white;
};

const float BLACK = 0.00017578;

void main() {
	vec3 color = subpassLoad(inputColor).rgb;
	color *= exposure;

	color = agx(color, white, BLACK);
	// color = agxLookPunchy(color);
	color = agxEotf(color);

	outFragColor = vec4(color, 1.0);
}
