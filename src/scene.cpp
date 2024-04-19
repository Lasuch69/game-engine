#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>

#include "loader.h"

#include "scene.h"

bool Scene::load(const std::filesystem::path &path, RenderingServer *pRenderingServer) {
	std::optional<Loader::Scene> result = Loader::loadGltf(path);

	if (!result.has_value())
		return false;

	Loader::Scene scene = result.value();

	for (const Loader::Material &sceneMaterial : scene.materials) {
		Texture albedoTexture = NULL_HANDLE;

		std::optional<size_t> albedoIndex = sceneMaterial.albedoIndex;
		if (albedoIndex.has_value()) {
			Image *pImage = scene.images[albedoIndex.value()];

			// Make sure we use RGBA8, because RGB8 is not suitable for textures.
			Image *pAlbedoImage = pImage->createRGBA8();
			albedoTexture = pRenderingServer->textureCreate(pAlbedoImage);
			free(pAlbedoImage);

			_textures.push_back(albedoTexture);
		}

		Material material = pRenderingServer->materialCreate(albedoTexture);
		_materials.push_back(material);
	}

	for (const Loader::Mesh &sceneMesh : scene.meshes) {
		std::vector<RS::Primitive> primitives;
		for (const Loader::Primitive &meshPrimitive : sceneMesh.primitives) {
			uint64_t materialIndex = meshPrimitive.materialIndex;
			Material material = _materials[materialIndex];

			RS::Primitive primitive = {};
			primitive.vertices = meshPrimitive.vertices;
			primitive.indices = meshPrimitive.indices;
			primitive.material = material;

			primitives.push_back(primitive);
		}

		Mesh mesh = pRenderingServer->meshCreate(primitives);
		_meshes.push_back(mesh);
	}

	for (const Loader::MeshInstance &sceneMeshInstance : scene.meshInstances) {
		uint64_t meshIndex = sceneMeshInstance.meshIndex;

		Mesh mesh = _meshes[meshIndex];
		glm::mat4 transform = sceneMeshInstance.transform;

		MeshInstance meshInstance = pRenderingServer->meshInstanceCreate();
		pRenderingServer->meshInstanceSetMesh(meshInstance, mesh);
		pRenderingServer->meshInstanceSetTransform(meshInstance, transform);

		_meshInstances.push_back(meshInstance);
	}

	for (const Loader::Light &sceneLight : scene.lights) {
		if (sceneLight.type != Loader::LightType::Point)
			continue;

		glm::vec3 position = glm::vec3(sceneLight.transform[3]);
		float range = sceneLight.range.value_or(0.0f);
		glm::vec3 color = sceneLight.color;
		float intensity = sceneLight.intensity;

		PointLight pointLight = pRenderingServer->pointLightCreate();
		pRenderingServer->pointLightSetPosition(pointLight, position);
		pRenderingServer->pointLightSetRange(pointLight, range);
		pRenderingServer->pointLightSetColor(pointLight, color);
		pRenderingServer->pointLightSetIntensity(pointLight, intensity);

		_pointLights.push_back(pointLight);
	}

	for (Image *pImage : scene.images) {
		free(pImage);
	}

	return true;
}

void Scene::clear(RenderingServer *pRenderingServer) {
	for (MeshInstance meshInstance : _meshInstances)
		pRenderingServer->meshInstanceFree(meshInstance);

	for (PointLight pointLight : _pointLights)
		pRenderingServer->pointLightFree(pointLight);

	for (Mesh mesh : _meshes)
		pRenderingServer->meshFree(mesh);

	for (Material material : _materials)
		pRenderingServer->materialFree(material);

	for (Texture texture : _textures)
		pRenderingServer->textureFree(texture);

	_meshInstances.clear();
	_pointLights.clear();
	_meshes.clear();
	_materials.clear();
	_textures.clear();
}
