struct Face {
	vec3 forward;
	vec3 up;
	vec3 right;
};

const Face FACE[6] = {
	// FACE +X - Front
	Face(
		vec3( 1.0,  0.0,  0.0),
		vec3( 0.0, -1.0,  0.0),
		vec3( 0.0,  0.0, -1.0)
	),
	// FACE -X - Back
	Face (
		vec3(-1.0,  0.0,  0.0),
		vec3( 0.0, -1.0,  0.0),
		vec3( 0.0,  0.0,  1.0)
	),
	// FACE +Y - Up
	Face (
		vec3( 0.0,  1.0,  0.0),
		vec3( 0.0,  0.0,  1.0),
		vec3( 1.0,  0.0,  0.0)
	),
	// FACE -Y - Down
	Face (
		vec3( 0.0, -1.0,  0.0),
		vec3( 0.0,  0.0, -1.0),
		vec3( 1.0,  0.0,  0.0)
	),
	// FACE +Z - Right
	Face (
		vec3( 0.0,  0.0,  1.0),
		vec3( 0.0, -1.0,  0.0),
		vec3( 1.0,  0.0,  0.0)
	),
	// FACE -Z - Left
	Face (
		vec3( 0.0,  0.0, -1.0),
		vec3( 0.0, -1.0,  0.0),
		vec3(-1.0,  0.0,  0.0)
	),
};

vec3 mapToCube(vec2 coords, uint faceIdx) {
	Face face = FACE[faceIdx];
	return normalize(face.forward + face.right * coords.x + face.up * coords.y);
}

vec2 mapToEquirectangular(vec3 coords) {
	const vec2 INV_ATAN = vec2(0.1591, 0.3183);
	return vec2(atan(coords.z, coords.x), asin(coords.y)) * INV_ATAN + 0.5;
}
