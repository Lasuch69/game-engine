#include <SDL2/SDL_events.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_mouse.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
#include <vulkan/vulkan.hpp>

#include <version.h>

#include "camera_controller.h"
#include "image_loader.h"
#include "scene.h"
#include "time.h"

#include "rendering/rendering_server.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

SDL_Window *windowInitialize(int argc, char *argv[]) {
	SDL_Window *pWindow = SDL_CreateWindow("Hayaku Engine", SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	if (pWindow == nullptr) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to create window!\n");
		return nullptr;
	}

	RS::getSingleton().initialize(argc, argv);

	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(pWindow, RS::getSingleton().getVkInstance(), &surface);

	int width, height;
	SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
	RS::getSingleton().windowInit(surface, width, height);

	return pWindow;
}

int main(int argc, char *argv[]) {
	printf("Hayaku Engine -- Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	printf("Author: Lasuch69 2024\n\n");

	SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");

	SDL_Init(SDL_INIT_VIDEO);
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

	SDL_Window *pWindow = windowInitialize(argc, argv);
	if (pWindow == nullptr) {
		return EXIT_FAILURE;
	}

	Scene *pScene = new Scene;

	for (int i = 1; i < argc; i++) {
		// --scene <path>
		if (strcmp("--scene", argv[i]) == 0 && i < argc - 1) {
			std::filesystem::path path(argv[i + 1]);
			pScene->load(path);
		}
	}

	Time time;
	CameraController camera;

	bool quit = false;
	while (!quit) {
		time.tick();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				quit = true;

			if (event.type == SDL_WINDOWEVENT) {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						int width, height;
						SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
						RS::getSingleton().windowResized(width, height);
						break;
					default:
						break;
				}
			}

			if (event.type == SDL_DROPFILE) {
				char *pFile = event.drop.file;
				std::filesystem::path path(pFile);
				SDL_free(pFile);

				const char *pExtension = path.extension().c_str();

				if (strcmp(pExtension, ".hdr") == 0) {
					std::shared_ptr<Image> image(ImageLoader::loadHDRFromFile(path));
					RS::getSingleton().environmentSkyUpdate(image);
					break;
				}

				if (strcmp(pExtension, ".exr") == 0) {
					std::shared_ptr<Image> image(ImageLoader::loadEXRFromFile(path));
					RS::getSingleton().environmentSkyUpdate(image);
					break;
				}

				pScene->clear();
				pScene->load(path);
			}

			if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
					case SDLK_F2:
						bool isRelative = SDL_GetRelativeMouseMode();
						SDL_SetRelativeMouseMode((SDL_bool)!isRelative);
						break;
				}
			}
		}

		camera.update(time.deltaTime());
		RS::getSingleton().draw();
	}

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
