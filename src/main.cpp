#include <SDL2/SDL_scancode.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <version.h>

#include "rendering/rendering_server.h"
#include "scene.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class CameraController {
	glm::mat4 _transform = glm::mat4(1.0f);

	glm::vec3 _translation = glm::vec3(0.0f);
	glm::vec3 _rotation = glm::vec3(0.0f);

	void _update() {
		_transform = glm::mat4(1.0f);

		_transform = glm::translate(_transform, _translation);
		_transform = glm::rotate(_transform, _rotation.x, glm::vec3(0.0, 1.0f, 0.0)); // rotate Y
		_transform = glm::rotate(_transform, _rotation.y, glm::vec3(1.0f, 0.0, 0.0)); // rotate X

		RS::getInstance().cameraSetTransform(_transform);
	}

public:
	void rotate(const glm::vec2 &input) {
		_rotation -= glm::vec3(input.x, input.y, 0.0);
		_rotation.y = glm::clamp(_rotation.y, glm::radians(-89.9f), glm::radians(89.9f));
		_update();
	}

	void move(const glm::vec2 &input) {
		_translation += glm::mat3(_transform) * glm::vec3(input.x, 0.0, -input.y);
		_update();
	}

	CameraController() {
		_update();
	}
};

int main(int argc, char *argv[]) {
	SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");

	printf("Hayaku Engine -- Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	printf("Author: Lasuch69 2024\n\n");

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *pWindow = SDL_CreateWindow("Hayaku Engine", SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	if (pWindow == nullptr) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to create window!\n");
		return EXIT_FAILURE;
	}

	RS::getInstance().initialize(argc, argv);

	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(pWindow, RS::getInstance().getVkInstance(), &surface);

	int width, height;
	SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
	RS::getInstance().windowInit(surface, width, height);

	Scene *pScene = new Scene;

	CameraController cameraController;

	for (int i = 1; i < argc; i++) {
		// --scene <path>
		if (strcmp("--scene", argv[i]) == 0 && i < argc - 1) {
			std::filesystem::path path(argv[i + 1]);
			pScene->load(path);
		}
	}

	uint64_t now = SDL_GetPerformanceCounter();
	uint64_t last = 0;
	float deltaTime = 0;

	SDL_SetRelativeMouseMode(SDL_TRUE);

	bool quit = false;
	while (!quit) {
		last = now;
		now = SDL_GetPerformanceCounter();

		deltaTime = ((now - last) / (float)SDL_GetPerformanceFrequency());

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			}
			if (event.type == SDL_WINDOWEVENT) {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						int width, height;
						SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
						RS::getInstance().windowResized(width, height);
						break;
					default:
						break;
				}
			}
			if (event.type == SDL_DROPFILE) {
				char *pFile = event.drop.file;
				std::filesystem::path path(pFile);
				SDL_free(pFile);

				pScene->clear();
				pScene->load(path);
			}
		}

		int x, y;
		SDL_GetRelativeMouseState(&x, &y);

		const uint8_t *keys = SDL_GetKeyboardState(nullptr);

		glm::vec2 input = {
			keys[SDL_SCANCODE_D] - keys[SDL_SCANCODE_A],
			keys[SDL_SCANCODE_W] - keys[SDL_SCANCODE_S],
		};

		input *= 3.0f;

		cameraController.rotate(glm::vec2(x, y) * 0.005f);
		cameraController.move(input * deltaTime);

		RS::getInstance().draw();
	}

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
