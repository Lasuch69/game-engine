struct Face {
	vec3 forward;
	vec3 up;
	vec3 right;
};

const Face FACE[6] = {
	// FACE +X - Front
	Face(
		vec3( 0.0,  0.0, -1.0),
		vec3( 0.0,  1.0,  0.0),
		vec3(-1.0,  0.0,  0.0)
	),
	// FACE -X - Back
	Face (
		vec3( 0.0,  0.0,  1.0),
		vec3( 0.0,  1.0,  0.0),
		vec3( 1.0,  0.0,  0.0)
	),
	// FACE +Y - Down
	Face (
		vec3( 0.0, -1.0,  0.0),
		vec3( 1.0,  0.0,  0.0),
		vec3( 0.0,  0.0, -1.0)
	),
	// FACE -Y - Up
	Face (
		vec3( 0.0,  1.0,  0.0),
		vec3(-1.0,  0.0,  0.0),
		vec3( 0.0,  0.0, -1.0)
	),
	// FACE +Z - Right
	Face (
		vec3( 1.0,  0.0,  0.0),
		vec3( 0.0,  1.0,  0.0),
		vec3( 0.0,  0.0, -1.0)
	),
	// FACE -Z - Left
	Face (
		vec3(-1.0,  0.0,  0.0),
		vec3( 0.0,  1.0,  0.0),
		vec3( 0.0,  0.0,  1.0)
	),
};

const Face FLIPPED_FACE[6] = {
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

vec3 mapToCube(vec2 coords, uint faceIdx, bool flipY) {
	Face face = FACE[faceIdx];

	if (flipY) {
		face = FLIPPED_FACE[faceIdx];
	}

	return normalize(face.forward + face.right * coords.x + face.up * coords.y);
}
