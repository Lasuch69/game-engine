#include <cstdint>
#include <cstdio>
#include <cstdlib>
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
	std::vector<MeshID> meshes;
	std::vector<MeshInstanceID> meshInstances;
	std::vector<PointLightID> pointLights;
	std::vector<TextureID> textures;
	std::vector<MaterialID> materials;
};

SceneData load(std::filesystem::path path, RenderingServer *pRS) {
	SceneData sceneData = {};

	Loader::Gltf *pGltf = Loader::loadGltf(path);

	for (std::shared_ptr<Image> pImage : pGltf->images) {
		TextureID texture = pRS->textureCreate(pImage.get());
		sceneData.textures.push_back(texture);
	}

	for (const Loader::Material &materialData : pGltf->materials) {
		TextureID albedoTexture = sceneData.textures[materialData.albedoIndex];
		MaterialID material = pRS->materialCreate(albedoTexture);
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
		MeshID meshID = pRS->meshCreate();

		for (const Loader::Primitive &primitive : mesh.primitives) {
			pRS->meshAddPrimitive(meshID, primitive.vertices, primitive.indices,
					sceneData.materials[primitive.materialIndex.value()]);
		}

		MeshInstanceID meshInstanceID = pRS->meshInstanceCreate();
		pRS->meshInstanceSetMesh(meshInstanceID, meshID);
		pRS->meshInstanceSetTransform(meshInstanceID, meshInstance.transform);

		sceneData.meshes.push_back(meshID);
		sceneData.meshInstances.push_back(meshInstanceID);
	}

	for (const Loader::PointLight &pointLight : pGltf->pointLights) {
		PointLightID pointLightID = pRS->pointLightCreate();
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
	for (MeshInstanceID meshInstance : sceneData.meshInstances) {
		pRS->meshInstanceFree(meshInstance);
	}

	for (PointLightID pointLight : sceneData.pointLights) {
		pRS->pointLightFree(pointLight);
	}

	for (MeshID mesh : sceneData.meshes) {
		pRS->meshFree(mesh);
	}

	for (MaterialID material : sceneData.materials) {
		pRS->materialFree(material);
	}

	for (TextureID texture : sceneData.textures) {
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

	RenderingServer *pRS = new RenderingServer;
	pRS->init(getRequiredExtensions(), true);

	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(pWindow, pRS->getVkInstance(), &surface);

	int width, height;
	SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
	pRS->windowInit(surface, width, height);

	std::optional<SceneData> sceneData = {};

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
				char *pFile = event.drop.file;

				SceneData newScene = load(pFile, pRS);

				if (sceneData.has_value()) {
					clear(sceneData.value(), pRS);
				}

				sceneData = newScene;

				SDL_free(pFile);
			}
		}

		pRS->draw();
	}

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
