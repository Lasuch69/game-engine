#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <optional>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <version.h>

#include "rendering/rendering_server.h"

#include "loader.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

struct SceneData {
	std::vector<Mesh> meshes;
	std::vector<MeshInstance> meshInstances;
	std::vector<PointLight> pointLights;
	std::vector<Texture> textures;
	std::vector<Material> materials;
};

SceneData load(std::filesystem::path path, RenderingServer *pRS) {
	SceneData sceneData = {};

	Loader::Gltf *pGltf = Loader::loadGltf(path);

	if (pGltf == nullptr) {
		return {};
	}

	for (std::shared_ptr<Image> pImage : pGltf->images) {
		Texture texture = pRS->textureCreate(pImage.get());
		sceneData.textures.push_back(texture);
	}

	for (const Loader::Material &materialData : pGltf->materials) {
		Texture albedoTexture = sceneData.textures[materialData.albedoIndex];
		Material material = pRS->materialCreate(albedoTexture);
		sceneData.materials.push_back(material);
	}

	for (const Loader::Camera &camera : pGltf->cameras) {
		pRS->cameraSetTransform(camera.transform);
		pRS->cameraSetFovY(camera.fovY);
		pRS->cameraSetZNear(camera.zNear);
		pRS->cameraSetZFar(camera.zFar);
	}

	for (const Loader::MeshInstance &meshInstance : pGltf->meshInstances) {
		Loader::Mesh mesh = pGltf->meshes[meshInstance.meshIndex];
		Mesh meshID = pRS->meshCreate();

		for (const Loader::Primitive &primitive : mesh.primitives) {
			pRS->meshAddPrimitive(meshID, primitive.vertices, primitive.indices,
					sceneData.materials[primitive.materialIndex.value()]);
		}

		MeshInstance meshInstanceID = pRS->meshInstanceCreate();
		pRS->meshInstanceSetMesh(meshInstanceID, meshID);
		pRS->meshInstanceSetTransform(meshInstanceID, meshInstance.transform);

		sceneData.meshes.push_back(meshID);
		sceneData.meshInstances.push_back(meshInstanceID);
	}

	for (const Loader::PointLight &pointLight : pGltf->pointLights) {
		PointLight pointLightID = pRS->pointLightCreate();
		pRS->pointLightSetPosition(pointLightID, pointLight.position);
		pRS->pointLightSetRange(pointLightID, pointLight.range);
		pRS->pointLightSetColor(pointLightID, pointLight.color);
		pRS->pointLightSetIntensity(pointLightID, pointLight.intensity);

		sceneData.pointLights.push_back(pointLightID);
	}

	free(pGltf);
	return sceneData;
}

void clear(SceneData sceneData, RenderingServer *pRS) {
	for (MeshInstance meshInstance : sceneData.meshInstances) {
		pRS->meshInstanceFree(meshInstance);
	}

	for (PointLight pointLight : sceneData.pointLights) {
		pRS->pointLightFree(pointLight);
	}

	for (Mesh mesh : sceneData.meshes) {
		pRS->meshFree(mesh);
	}

	for (Material material : sceneData.materials) {
		pRS->materialFree(material);
	}

	for (Texture texture : sceneData.textures) {
		pRS->textureFree(texture);
	}
}

std::vector<const char *> getRequiredExtensions() {
	uint32_t extensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);

	std::vector<const char *> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

	return extensions;
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

	RS *pRS = new RS(argc, argv);
	pRS->init(getRequiredExtensions());

	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(pWindow, pRS->getVkInstance(), &surface);

	int width, height;
	SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
	pRS->windowInit(surface, width, height);

	std::optional<SceneData> sceneData = {};

	for (int i = 1; i < argc; i++) {
		// --scene <path>
		if (strcmp("--scene", argv[i]) == 0 && i < argc - 1) {
			std::filesystem::path path(argv[i + 1]);
			sceneData = load(path, pRS);
		}
	}

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
						pRS->windowResized(width, height);
						break;
					default:
						break;
				}
			}
			if (event.type == SDL_DROPFILE) {
				char *pPath = event.drop.file;

				if (sceneData.has_value()) {
					clear(sceneData.value(), pRS);
				}

				sceneData = load(pPath, pRS);

				SDL_free(pPath);
			}
		}

		pRS->draw();
	}

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
