#include <cstdint>
#include <cstdlib>
#include <cstring>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>

#include <io/compression.h>
#include <io/image_loader.h>

#include "camera_controller.h"
#include "rendering/rendering_server.h"
#include "scene.h"
#include "timer.h"

typedef struct {
	SDL_Window *pWindow;

	CameraController camera;
	Timer timer;
	Scene scene;
} AppState;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int SDL_AppInit(void **appstate, int argc, char **argv) {
	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");

	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
	SDL_Window *pWindow = SDL_CreateWindow("Hayaku Engine", WIDTH, HEIGHT, flags);

	if (pWindow == nullptr) {
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());
		return -1;
	}

	RS::getSingleton().initialize(argc, argv);
	RS::getSingleton().windowInit(pWindow);

	AppState *pState = new AppState;
	pState->pWindow = pWindow;

	for (int i = 1; i < argc; i++) {
		// --scene <path>
		if (strcmp("--scene", argv[i]) == 0 && i < argc - 1) {
			const char *pFile = argv[i + 1];
			pState->scene.load(pFile);
		}
	}

	appstate[0] = reinterpret_cast<void *>(pState);
	return 0;
}

int SDL_AppIterate(void *appstate) {
	AppState *pState = reinterpret_cast<AppState *>(appstate);

	pState->timer.tick();

	float deltaTime = pState->timer.deltaTime();
	pState->camera.update(deltaTime);

	RS::getSingleton().draw();

	return 0;
}

int SDL_AppEvent(void *appstate, const SDL_Event *event) {
	AppState *pState = reinterpret_cast<AppState *>(appstate);

	if (event->type == SDL_EVENT_QUIT)
		return 1;

	if (event->type == SDL_EVENT_WINDOW_RESIZED) {
		int width, height;
		SDL_GetWindowSizeInPixels(pState->pWindow, &width, &height);
		RS::getSingleton().windowResized(width, height);
		return 0;
	}

	if (event->type == SDL_EVENT_DROP_FILE) {
		const char *pFile = event->drop.data;

		if (ImageLoader::isImage(pFile)) {
			std::shared_ptr<Image> image = ImageLoader::loadFromFile(pFile);
			Compression::imageCompress(image.get());

			RS::getSingleton().environmentSkyUpdate(image);
			return 0;
		}

		pState->scene.clear();
		pState->scene.load(pFile);
		return 0;
	}

	if (event->type == SDL_EVENT_KEY_DOWN && event->key.keysym.sym == SDLK_F2) {
		SDL_SetRelativeMouseMode(!SDL_GetRelativeMouseMode());
		return 0;
	}

	return 0;
}

void SDL_AppQuit(void *appstate) {
	AppState *pState = reinterpret_cast<AppState *>(appstate);
	SDL_DestroyWindow(pState->pWindow);
	free(pState);
}
