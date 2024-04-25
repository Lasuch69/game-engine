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
		Texture normalTexture = NULL_HANDLE;
		Texture roughnessTexture = NULL_HANDLE;

		std::optional<size_t> albedoIndex = sceneMaterial.albedoIndex;
		if (albedoIndex.has_value()) {
			Image *pImage = scene.images[albedoIndex.value()];

			// Make sure we use RGBA8, because RGB8 is not suitable for textures.
			Image *pAlbedoImage = pImage->createRGBA8();
			albedoTexture = pRenderingServer->textureCreate(pAlbedoImage);
			free(pAlbedoImage);

			_textures.push_back(albedoTexture);
		}

		std::optional<size_t> normalIndex = sceneMaterial.normalIndex;
		if (normalIndex.has_value()) {
			Image *pImage = scene.images[normalIndex.value()];

			Image *pNormalImage = pImage->createRG8();
			normalTexture = pRenderingServer->textureCreate(pNormalImage);
			free(pNormalImage);

			_textures.push_back(normalTexture);
		}

		std::optional<size_t> roughnessIndex = sceneMaterial.roughnessIndex;
		if (roughnessIndex.has_value()) {
			Image *pImage = scene.images[roughnessIndex.value()];

			Image *pRoughnessImage = pImage->createR8();
			roughnessTexture = pRenderingServer->textureCreate(pRoughnessImage);
			free(pRoughnessImage);

			_textures.push_back(roughnessTexture);
		}

		Material material =
				pRenderingServer->materialCreate(albedoTexture, normalTexture, roughnessTexture);
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
		if (sceneLight.type == Loader::LightType::Point) {
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

			continue;
		}

		if (sceneLight.type == Loader::LightType::Directional) {
			glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
			glm::mat3 rotation = glm::mat3(sceneLight.transform);

			glm::vec3 direction = rotation * front;
			glm::vec3 color = sceneLight.color;
			float intensity = sceneLight.intensity;

			DirectionalLight directionalLight = pRenderingServer->directionalLightCreate();
			pRenderingServer->directionalLightSetDirection(directionalLight, direction);
			pRenderingServer->directionalLightSetIntensity(directionalLight, intensity);
			pRenderingServer->directionalLightSetColor(directionalLight, color);

			_directionalLights.push_back(directionalLight);

			continue;
		}
	}

	for (Image *pImage : scene.images) {
		free(pImage);
	}

	return true;
}

void Scene::clear(RenderingServer *pRenderingServer) {
	for (MeshInstance meshInstance : _meshInstances)
		pRenderingServer->meshInstanceFree(meshInstance);

	for (DirectionalLight directionalLight : _directionalLights)
		pRenderingServer->directionalLightFree(directionalLight);

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
