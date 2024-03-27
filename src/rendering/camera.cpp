#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include "camera.h"

glm::mat4 Camera::viewMatrix() const {
	glm::vec3 position = glm::vec3(transform[3]);
	glm::mat3 rotation = glm::mat3(transform);

	glm::vec3 front = glm::normalize(rotation * CAMERA_FRONT);
	return glm::lookAtRH(position, position + front, CAMERA_UP);
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
	glm::mat4 projection = glm::perspective(fovY, aspect, zNear, zFar);
	projection[1][1] *= -1;

	return projection;
}
