#version 450

#extension GL_EXT_multiview : enable

#include "include/cubemap_incl.glsl"

layout(location = 0) in vec2 coords;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform samplerCube cubeSampler;

const float PI = 3.1415926535;

void main() {
	// the sample direction equals the hemisphere's orientation
	vec3 N = mapToCube(coords, gl_ViewIndex, true);
	vec3 irradiance = vec3(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = normalize(cross(N, right));

	float sampleDelta = 0.025;
	float nrSamples = 0.0;

	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
			// spherical to cartesian (in tangent space)
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			// tangent space to world
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			irradiance += texture(cubeSampler, sampleVec).rgb * cos(theta) * sin(theta);
			nrSamples++;
		}
	}

	irradiance = PI * irradiance * (1.0 / float(nrSamples));
	fragColor = vec4(irradiance, 1.0);
}
