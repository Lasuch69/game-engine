#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <version.h>

#include "rendering/rendering_server.h"

#include "loader.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

void load(std::filesystem::path path, RenderingServer *pRS) {
	std::vector<Loader::Node> nodes = Loader::loadScene(path);

	for (const Loader::Node &node : nodes) {
		bool hasCamera = node.camera.has_value();
		if (hasCamera) {
			Loader::Camera cameraData = node.camera.value();
			pRS->cameraSetFovY(cameraData.fovY);
			pRS->cameraSetZNear(cameraData.zNear);
			pRS->cameraSetZFar(cameraData.zFar);
			pRS->cameraSetTransform(node.transform);
		}

		bool hasMesh = node.mesh.has_value();
		if (hasMesh) {
			Loader::Mesh meshData = node.mesh.value();
			RS::Mesh mesh = pRS->meshCreate(meshData.vertices, meshData.indices);

			RS::MeshInstance meshInstance = pRS->meshInstanceCreate();
			pRS->meshInstanceSetMesh(meshInstance, mesh);
			pRS->meshInstanceSetTransform(meshInstance, node.transform);
		}

		bool hasLight = node.pointLight.has_value();
		if (hasLight) {
			Loader::PointLight lightData = node.pointLight.value();
			glm::vec3 position = glm::vec3(node.transform[3]);

			RS::Light light = pRS->lightCreate();
			pRS->lightSetPosition(light, position);
			pRS->lightSetRange(light, lightData.range);
			pRS->lightSetColor(light, lightData.color);
			pRS->lightSetIntensity(light, lightData.intensity);
		}
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

	RenderingServer *pRS = new RenderingServer(pWindow);

	std::vector<MeshInstance> meshInstances;

	bool quit = false;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			}
			if (event.type == SDL_DROPFILE) {
				char *pFile = event.drop.file;
				load(pFile, pRS);
				SDL_free(pFile);
			}
		}

		pRS->draw();
	}

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
