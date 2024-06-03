#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>

#include <SDL3/SDL.h>

#include "rendering/rendering_server.h"

#include "camera_controller.h"

void CameraController::_update() {
	_transform = glm::mat4(1.0f);
	_transform = glm::translate(_transform, _translation);
	_transform = glm::rotate(_transform, _rotation.x, glm::vec3(0.0, 1.0f, 0.0)); // rotate Y
	_transform = glm::rotate(_transform, _rotation.y, glm::vec3(1.0f, 0.0, 0.0)); // rotate X

	RS::getSingleton().cameraSetTransform(_transform);
}

void CameraController::_rotate(const glm::vec2 &input) {
	_rotation -= glm::vec3(input.x, input.y, 0.0);
	_rotation.y = glm::clamp(_rotation.y, glm::radians(-89.9f), glm::radians(89.9f));
	_update();
}

void CameraController::_move(const glm::vec2 &input) {
	_translation += glm::mat3(_transform) * glm::vec3(input.x, 0.0, -input.y);
	_update();
}

void CameraController::update(float deltaTime) {
	if (SDL_GetRelativeMouseMode() == false) {
		_reset = true;
		return;
	}

	float x, y;
	SDL_GetRelativeMouseState(&x, &y);

	if (_reset) {
		x = 0;
		y = 0;

		_reset = false;
	}

	_rotate(glm::vec2(x, y) * 0.005f);

	const uint8_t *keys = SDL_GetKeyboardState(nullptr);

	glm::vec2 input = {
		keys[SDL_SCANCODE_D] - keys[SDL_SCANCODE_A],
		keys[SDL_SCANCODE_W] - keys[SDL_SCANCODE_S],
	};

	glm::vec2 velocity = input * SPEED * deltaTime;
	_move(velocity);
}

CameraController::CameraController() {
	_update();
}
