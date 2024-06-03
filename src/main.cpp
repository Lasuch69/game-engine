#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_video.h>

#include <version.h>

#include "camera_controller.h"
#include "image_loader.h"
#include "scene.h"
#include "time.h"

#include "rendering/rendering_server.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

void loadFile(Scene *pScene, char *pFile) {
	std::filesystem::path path(pFile);
	const char *pExtension = path.extension().c_str();

	if (strcmp(pExtension, ".hdr") == 0) {
		std::shared_ptr<Image> image(ImageLoader::loadHDRFromFile(path));
		RS::getSingleton().environmentSkyUpdate(image);
		return;
	}

	if (strcmp(pExtension, ".exr") == 0) {
		std::shared_ptr<Image> image(ImageLoader::loadEXRFromFile(path));
		RS::getSingleton().environmentSkyUpdate(image);
		return;
	}

	pScene->clear();
	pScene->load(path);
}

int main(int argc, char *argv[]) {
	printf("Hayaku Engine -- Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	printf("Author: Lasuch69 2024\n\n");

	SDL_Init(SDL_INIT_VIDEO);
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

	SDL_Window *pWindow = SDL_CreateWindow(
			"Hayaku Engine", WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	if (pWindow == nullptr) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to create window!\n");
		return EXIT_FAILURE;
	}

	RS::getSingleton().initialize(argc, argv);
	RS::getSingleton().windowInit(pWindow);

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
			if (event.type == SDL_EVENT_QUIT)
				quit = true;

			if (event.type == SDL_EVENT_WINDOW_RESIZED) {
				int width, height;
				SDL_GetWindowSizeInPixels(pWindow, &width, &height);

				RS::getSingleton().windowResized(width, height);
			}

			if (event.type == SDL_EVENT_DROP_FILE) {
				char *pFile = event.drop.data;
				loadFile(pScene, pFile);
			}

			if (event.type == SDL_EVENT_KEY_DOWN) {
				if (event.key.keysym.sym != SDLK_F2)
					continue;

				bool isRelative = SDL_GetRelativeMouseMode();
				SDL_SetRelativeMouseMode((SDL_bool)!isRelative);
			}
		}

		camera.update(time.deltaTime());
		RS::getSingleton().draw();
	}

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
