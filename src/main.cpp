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

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO *pIo = &ImGui::GetIO();
	pIo->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	pIo->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	pIo->ConfigDebugIsDebuggerPresent = true;

	ImGui::StyleColorsDark();

	ImGui_ImplSDL2_InitForVulkan(pWindow);

	RS::getSingleton().initImGui();

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

		static bool isGuiVisible = false;

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

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
					case SDLK_F1:
						isGuiVisible = !isGuiVisible;
						break;
					case SDLK_F2:
						bool isRelative = SDL_GetRelativeMouseMode();
						SDL_SetRelativeMouseMode((SDL_bool)!isRelative);
						break;
				}
			}
		}

		camera.update(time.deltaTime());

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		if (isGuiVisible) {
			ImGui::Begin("ImGui", &isGuiVisible);

			if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen)) {
				static bool enabled = false;
				bool toggled = false;

				{
					bool _enable = enabled;
					ImGui::Checkbox("Enable", &enabled);
					toggled = _enable != enabled;
				}

				static float direction[3] = { -45.0f, 60.0f, 0.0f };
				static float color[3] = { 1.0f, 1.0f, 1.0f };
				static float intensity = 1.0f;

				ImGui::DragFloat3("Direction", direction, 0.1f, -720.0f, 720.0f, "%.1f");
				ImGui::ColorEdit3("Color", color);
				ImGui::DragFloat("Intensity", &intensity, 0.05f, 0.0f, 128.0f, "%.2f");

				static ObjectID sun;

				if (toggled) {
					if (enabled) {
						sun = RS::getSingleton().lightCreate(LightType::Directional);
					} else {
						RS::getSingleton().lightFree(sun);
					}
				}

				if (enabled) {
					glm::vec3 d = glm::make_vec3(direction);
					d.x = glm::radians(d.x);
					d.y = glm::radians(d.y);
					d.z = glm::radians(d.z);

					glm::mat4 t(1.0f);
					t = glm::rotate(t, d.y, glm::vec3(0.0, 1.0, 0.0)); // rotate X
					t = glm::rotate(t, d.x, glm::vec3(1.0, 0.0, 0.0)); // rotate Y

					RS::getSingleton().lightSetTransform(sun, t);
					RS::getSingleton().lightSetColor(sun, glm::make_vec3(color));
					RS::getSingleton().lightSetIntensity(sun, intensity);
				}
			}

			if (ImGui::CollapsingHeader("Tonemapping", ImGuiTreeNodeFlags_DefaultOpen)) {
				static float exposure = 1.25f;
				static float white = 8.0f;

				ImGui::DragFloat("Exposure", &exposure, 0.01f, 0.0f, 10.0f, "%.2f");
				ImGui::DragFloat("White", &white, 0.01f, 0.0f, 16.0f, "%.2f");

				RS::getSingleton().setExposure(exposure);
				RS::getSingleton().setWhite(white);
			}

			if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				static int currentItem = 0;

				const char *items[] = {
					"Mailbox",
					"Fifo",
					"Immediate",
				};

				ImGui::Combo("Present Mode", &currentItem, items, 3);
			}

			if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
				float frameTime = time.deltaTime() * 1000.0;

				ImGui::Text("FPS: %d (%0.1fms)", (int)(1000.0f / frameTime), frameTime);
			}

			ImGui::End();
		}

		ImGui::Render();

		RS::getSingleton().draw();
	}

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
