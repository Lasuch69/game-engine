#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <glm/fwd.hpp>
#include <variant>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/types.hpp>

#include "rendering/rendering_device.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const float STERADIAN = glm::pi<float>() * 4.0f;

glm::mat4 getTransformMatrix(const fastgltf::Node &node, glm::mat4x4 &base) {
	/** Both a matrix and TRS values are not allowed
	 * to exist at the same time according to the spec */
	if (const auto *pMatrix = std::get_if<fastgltf::Node::TransformMatrix>(&node.transform)) {
		return base * glm::mat4x4(glm::make_mat4x4(pMatrix->data()));
	}

	if (const auto *pTransform = std::get_if<fastgltf::TRS>(&node.transform)) {
		// Warning: The quaternion to mat4x4 conversion here is not correct with all versions of
		// glm. glTF provides the quaternion as (x, y, z, w), which is the same layout glm used up
		// to version 0.9.9.8. However, with commit 59ddeb7 (May 2021) the default order was changed
		// to (w, x, y, z). You could either define GLM_FORCE_QUAT_DATA_XYZW to return to the old
		// layout, or you could use the recently added static factory constructor glm::quat::wxyz(w,
		// x, y, z), which guarantees the parameter order.
		return base *
				glm::translate(glm::mat4(1.0f), glm::make_vec3(pTransform->translation.data())) *
				glm::toMat4(glm::make_quat(pTransform->rotation.data())) *
				glm::scale(glm::mat4(1.0f), glm::make_vec3(pTransform->scale.data()));
	}

	return base;
}

void load(std::filesystem::path path, RenderingDevice *pDevice) {
	fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);

	fastgltf::GltfDataBuffer data;
	data.loadFromFile(path);

	auto asset = parser.loadGltf(&data, path.parent_path(), fastgltf::Options::None);
	if (asset.error() != fastgltf::Error::None) {
		printf("Error: %d; %s\n", (int)asset.error(), path.c_str());
	}

	for (const fastgltf::Node &node : asset->nodes) {
		glm::mat4 base = glm::mat4(1.0f);
		glm::mat4 transform = getTransformMatrix(node, base);

		bool hasCamera = node.cameraIndex.has_value();
		if (hasCamera) {
			fastgltf::Camera camera = asset->cameras[node.cameraIndex.value()];

			// c++'s "features"; WHY?!?!?
			std::visit(fastgltf::visitor{
							   [&](fastgltf::Camera::Perspective &perspective) {
								   pDevice->createCamera(transform, glm::degrees(perspective.yfov),
										   perspective.znear, perspective.zfar.value_or(100.0));
							   },
							   [&](fastgltf::Camera::Orthographic &orthographic) {} },
					camera.camera);

			continue;
		}

		bool hasLight = node.lightIndex.has_value();
		if (hasLight) {
			fastgltf::Light light = asset->lights[node.lightIndex.value()];

			if (light.type != fastgltf::LightType::Point)
				continue;

			glm::vec3 position = glm::vec3(transform[3]);

			float intensity = light.intensity;
			intensity *= STERADIAN; // candela to lumen
			intensity /= 1000; // lumen to arbitrary

			pDevice->createLight(position, glm::make_vec3(light.color.data()), intensity);
			continue;
		}

		bool hasMesh = node.meshIndex.has_value();
		if (!hasMesh)
			continue;

		fastgltf::Mesh mesh = asset->meshes[node.meshIndex.value()];
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const fastgltf::Primitive &primitive : mesh.primitives) {
			if (primitive.indicesAccessor.has_value()) {
				auto &accessor = asset->accessors[primitive.indicesAccessor.value()];
				indices.resize(accessor.count);

				std::size_t idx = 0;
				fastgltf::iterateAccessor<std::uint32_t>(asset.get(), accessor,
						[&](std::uint32_t index) { indices[idx++] = index; });
			}

			auto *positionIter = primitive.findAttribute("POSITION");
			auto &positionAccessor = asset->accessors[positionIter->second];
			if (!positionAccessor.bufferViewIndex.has_value())
				continue;

			vertices.resize(positionAccessor.count);

			fastgltf::iterateAccessorWithIndex<glm::vec3>(
					asset.get(), positionAccessor, [&](glm::vec3 position, std::size_t idx) {
						vertices[idx].position = position;

						// default color = (1.0, 1.0, 1.0)
						vertices[idx].color = glm::vec3(1.0f);
					});

			auto *normalIter = primitive.findAttribute("NORMAL");
			auto &normalAccessor = asset->accessors[normalIter->second];
			if (!normalAccessor.bufferViewIndex.has_value())
				continue;

			fastgltf::iterateAccessorWithIndex<glm::vec3>(asset.get(), normalAccessor,
					[&](glm::vec3 normal, std::size_t idx) { vertices[idx].normal = normal; });
		}

		pDevice->createMesh(vertices, indices, transform);
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
