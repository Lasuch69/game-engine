#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_multiview : enable

#include "include/cubemap_incl.glsl"

layout(location = 0) in vec2 inCoords;
layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform samplerCube cubeSampler;

const float PI = 3.1415926535;

void main() {
	// the sample direction equals the hemisphere's orientation
	vec3 n = mapToCube(inCoords, gl_ViewIndex, true);
	vec3 irradiance = vec3(0.0);

	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, n));
	up = normalize(cross(n, right));

	float sampleDelta = 0.025;
	float sampleCount = 0.0;

	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
			// spherical to cartesian (in tangent space)
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

			// tangent space to world
			vec3 coords = tangentSample.x * right + tangentSample.y * up + tangentSample.z * n;

			irradiance += texture(cubeSampler, coords).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}

	irradiance = PI * irradiance * (1.0 / float(sampleCount));
	outFragColor = vec4(irradiance, 1.0);
}
