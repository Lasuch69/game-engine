#version 450

layout(location = 0) out vec2 outCoords;

const vec2 POSITIONS[3] = vec2[](
	vec2(-1.0, -1.0),
	vec2(-1.0, 3.0),
	vec2(3.0, -1.0)
);

void main() {
	vec4 position = vec4(POSITIONS[gl_VertexIndex], 0.0, 1.0);

	outCoords = position.xy;
	gl_Position = position;
}
