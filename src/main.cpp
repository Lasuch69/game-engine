#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <version.h>

#include "rendering/rendering_device.h"

#include "loader.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

std::vector<const char *> getRequiredExtensions() {
	uint32_t extensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);

	std::vector<const char *> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

	return extensions;
}

void load(std::filesystem::path path, RenderingDevice *pRenderingDevice) {
	std::vector<Node> nodes = Loader::loadScene(path);

	for (const Node &node : nodes) {
		bool hasCamera = node.camera.has_value();
		if (hasCamera)
			pRenderingDevice->createCamera(node.transform, glm::degrees(node.camera->yfov),
					node.camera->znear, node.camera->zfar.value_or(100.0f));

		bool hasMesh = node.mesh.has_value();
		if (hasMesh)
			pRenderingDevice->createMeshInstance(&node.mesh.value(), node.transform);

		bool hasPointLight = node.pointLight.has_value();
		if (hasPointLight)
			pRenderingDevice->createPointLight(
					&node.pointLight.value(), glm::vec3(node.transform[3]));
	}
}

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

	RenderingDevice *pRenderingDevice = new RenderingDevice(getRequiredExtensions(), true);

	VkSurfaceKHR surface;
	SDL_bool result = SDL_Vulkan_CreateSurface(pWindow, pRenderingDevice->getInstance(), &surface);

	if (result == SDL_FALSE) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to create surface!\n");
		return EXIT_FAILURE;
	}

	int width, height;
	SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
	pRenderingDevice->createWindow(static_cast<vk::SurfaceKHR>(surface),
			static_cast<uint32_t>(width), static_cast<uint32_t>(height));

	bool quit = false;
	while (!quit) {
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
						pRenderingDevice->resizeWindow(width, height);
						break;
					default:
						break;
				}
			}
			if (event.type == SDL_DROPFILE) {
				char *pFile = event.drop.file;
				load(pFile, pRenderingDevice);
				SDL_free(pFile);
			}
		}

		pRenderingDevice->draw();
	}

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
