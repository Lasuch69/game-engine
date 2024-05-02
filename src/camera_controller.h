#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include <SDL2/SDL.h>
#include <glm/glm.hpp>

class CameraController {
public:
	const float SPEED = 3.0f;

private:
	glm::vec3 _translation = glm::vec3(0.0f);
	glm::vec3 _rotation = glm::vec3(0.0f);
	glm::mat4 _transform = glm::mat4(1.0f);

	void _update();
	void _rotate(const glm::vec2 &input);
	void _move(const glm::vec2 &input);

public:
	void update(float deltaTime);

	CameraController();
};

#endif // !CAMERA_CONTROLLER_H
