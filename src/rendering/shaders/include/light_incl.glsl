struct DirectionalLight {
	vec3 direction;
	float _padding;

	vec3 color;
	float intensity;
};

struct PointLight {
	vec3 position;
	float range;

	vec3 color;
	float intensity;
};
