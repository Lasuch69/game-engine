#version 450

#include "include/tonemap_incl.glsl"

layout(push_constant) uniform TonemapParameterConstants {
	float exposure;
	float white;
};

layout(set = 0, binding = 0, input_attachment_index = 0) uniform subpassInput inputColor;

layout(location = 0) out vec4 fragColor;

const float black = 0.00017578;

void main() {
	vec3 color = subpassLoad(inputColor).rgb;
	color *= exposure;

	color = agx(color, white, black);
	// color = agxLookPunchy(color);
	color = agxEotf(color);

	fragColor = vec4(color, 1.0);
}
