#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include <version.h>

#include "camera_controller.h"
#include "scene.h"
#include "time.h"

#include "rendering/rendering_server.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

SDL_Window *windowInitialize(int argc, char *argv[]) {
	SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");
	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *pWindow = SDL_CreateWindow("Hayaku Engine", SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	if (pWindow == nullptr) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to create window!\n");
		return nullptr;
	}

	RS::getInstance().initialize(argc, argv);

	VkSurfaceKHR surface;
	SDL_Vulkan_CreateSurface(pWindow, RS::getInstance().getVkInstance(), &surface);

	int width, height;
	SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
	RS::getInstance().windowInit(surface, width, height);

	return pWindow;
}

int main(int argc, char *argv[]) {
	printf("Hayaku Engine -- Version %d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
	printf("Author: Lasuch69 2024\n\n");

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

	RS::getInstance().initImGui();

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

			if (ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
				static float color[4];
				static float value = 0.5f;

				ImGui::ColorEdit4("Test Color", color);
				ImGui::SliderFloat("Value", &value, 0.0f, 1.0f);
			}

			if (ImGui::CollapsingHeader("Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
				{
					static int samples = RS::getInstance().MSAASamplesGet();
					const int _samples = samples;

					const char *pItems[] = {
						"X1",
						"X2",
						"X4",
						"X8",
					};

					ImGui::Combo("MSAA", &samples, pItems, MSAA_MAX);

					if (_samples != samples)
						RS::getInstance().MSAASamplesSet(static_cast<MSAA>(samples));
				}

				{
					static int presentMode = 0;

					const char *pItems[] = {
						"Mailbox",
						"Fifo",
						"Immediate",
					};

					ImGui::Combo("Present Mode", &presentMode, pItems, 3);
				}
			}

			if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
				float frameTime = time.deltaTime() * 1000.0;

				ImGui::Text("FPS: %d (%0.1fms)", (int)(1000.0f / frameTime), frameTime);
			}

			ImGui::End();
		}

		ImGui::Render();

		RS::getInstance().draw();
	}

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
