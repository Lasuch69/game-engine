#version 450

const vec2 positions[3] = vec2[](
	vec2(-1.0, -1.0),
	vec2(-1.0, 3.0),
	vec2(3.0, -1.0)
);

layout(location = 0) out vec2 coords;

void main() {
	vec4 position = vec4(positions[gl_VertexIndex], 0.0, 1.0);

	coords = position.xy;
	gl_Position = position;
}
