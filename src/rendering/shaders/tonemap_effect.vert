#version 450

const vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0, 3.0),
    vec2(3.0, -1.0)
);

layout(location = 0) out vec2 outCoords;

void main() {
	vec4 position = vec4(positions[gl_VertexIndex], 0.0, 1.0);

	outCoords = (position.xy + vec2(1.0)) / 2.0;
	gl_Position = position;
}
