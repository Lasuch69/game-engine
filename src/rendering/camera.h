#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

const glm::vec3 CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 CAMERA_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);

class Camera {
public:
	glm::mat4 transform = glm::mat4(1.0f);
	float fovY = 60.0f;
	float zNear = 0.1f;
	float zFar = 100.0f;

	glm::mat4 viewMatrix() const;
	glm::mat4 projectionMatrix(float aspect) const;
};

#endif // !CAMERA_H
