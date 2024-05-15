#ifndef CAMERA_H
#define CAMERA_H

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

const glm::vec3 CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 CAMERA_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);

const glm::mat4 OPENGL_TO_VULKAN_MATRIX = {
	glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
	glm::vec4(0.0f, -1.0f, 0.0f, 0.0f),
	glm::vec4(0.0f, 0.0f, 0.5f, 0.0f),
	glm::vec4(0.0f, 0.0f, 0.5f, 1.0f),
};

const glm::mat4 REVERSE_Z_MATRIX = {
	glm::vec4(1.0f, 0.0f, 0.0f, 0.0f),
	glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
	glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
	glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
};

struct Camera {
	glm::mat4 transform = glm::mat4(1.0f);
	float fovY = glm::radians(60.0f);
	float zNear = 0.1f;
	float zFar = 100.0f;

	glm::mat4 viewMatrix() const {
		glm::vec3 position = glm::vec3(transform[3]);
		glm::mat3 rotation = glm::mat3(transform);

		glm::vec3 front = glm::normalize(rotation * CAMERA_FRONT);
		return glm::lookAtRH(position, position + front, CAMERA_UP);
	}

	glm::mat4 projectionMatrix(float aspect) const {
		glm::mat4 projection = glm::perspectiveRH(fovY, aspect, zNear, zFar);
		projection = REVERSE_Z_MATRIX * OPENGL_TO_VULKAN_MATRIX * projection;

		return projection;
	};
};

#endif // !CAMERA_H
