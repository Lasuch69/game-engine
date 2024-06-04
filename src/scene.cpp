#include <cstdint>
#include <filesystem>
#include <optional>

#include "io/asset_loader.h"
#include "rendering/rendering_server.h"

#include "scene.h"

bool Scene::load(const std::filesystem::path &path) {
	AssetLoader::Scene scene = AssetLoader::loadGltf(path);

	for (const AssetLoader::Material &sceneMaterial : scene.materials) {
		RS::MaterialInfo info;

		std::optional<size_t> albedoIndex = sceneMaterial.albedoIndex;
		if (albedoIndex.has_value()) {
			std::shared_ptr<Image> albedoMap = scene.images[albedoIndex.value()];

			ObjectID t = RS::getSingleton().textureCreate(albedoMap);
			_textures.push_back(t);

			info.albedo = t;
		}

		std::optional<size_t> normalIndex = sceneMaterial.normalIndex;
		if (normalIndex.has_value()) {
			std::shared_ptr<Image> normalMap = scene.images[normalIndex.value()];

			ObjectID t = RS::getSingleton().textureCreate(normalMap);
			_textures.push_back(t);

			info.normal = t;
		}

		std::optional<size_t> metallicIndex = sceneMaterial.metallicIndex;
		if (metallicIndex.has_value()) {
			std::shared_ptr<Image> metallicMap = scene.images[metallicIndex.value()];

			ObjectID t = RS::getSingleton().textureCreate(metallicMap);
			_textures.push_back(t);

			info.metallic = t;
		}

		std::optional<size_t> roughnessIndex = sceneMaterial.roughnessIndex;
		if (roughnessIndex.has_value()) {
			std::shared_ptr<Image> roughnessMap = scene.images[roughnessIndex.value()];

			ObjectID t = RS::getSingleton().textureCreate(roughnessMap);
			_textures.push_back(t);

			info.roughness = t;
		}

		ObjectID material = RS::getSingleton().materialCreate(info);
		_materials.push_back(material);
	}

	for (const AssetLoader::Mesh &sceneMesh : scene.meshes) {
		std::vector<RS::Primitive> primitives;
		for (const AssetLoader::Primitive &meshPrimitive : sceneMesh.primitives) {
			uint64_t materialIndex = meshPrimitive.materialIndex;
			ObjectID material = _materials[materialIndex];

			RS::Primitive primitive = {};
			primitive.vertices = meshPrimitive.vertices;
			primitive.indices = meshPrimitive.indices;
			primitive.material = material;

			primitives.push_back(primitive);
		}

		ObjectID mesh = RS::getSingleton().meshCreate(primitives);
		_meshes.push_back(mesh);
	}

	for (const AssetLoader::MeshInstance &sceneMeshInstance : scene.meshInstances) {
		uint64_t meshIndex = sceneMeshInstance.meshIndex;

		ObjectID mesh = _meshes[meshIndex];
		glm::mat4 transform = sceneMeshInstance.transform;

		ObjectID meshInstance = RS::getSingleton().meshInstanceCreate();
		RS::getSingleton().meshInstanceSetMesh(meshInstance, mesh);
		RS::getSingleton().meshInstanceSetTransform(meshInstance, transform);

		_meshInstances.push_back(meshInstance);
	}

	for (const AssetLoader::Light &sceneLight : scene.lights) {
		glm::mat4 transform = sceneLight.transform;
		float range = sceneLight.range.value_or(0.0f);
		glm::vec3 color = sceneLight.color;
		float intensity = sceneLight.intensity;

		ObjectID light;

		switch (sceneLight.type) {
			case AssetLoader::LightType::Directional:
				light = RS::getSingleton().lightCreate(LightType::Directional);
				break;
			case AssetLoader::LightType::Point:
				light = RS::getSingleton().lightCreate(LightType::Point);
				break;
		}

		RS::getSingleton().lightSetTransform(light, transform);
		RS::getSingleton().lightSetRange(light, range);
		RS::getSingleton().lightSetColor(light, color);
		RS::getSingleton().lightSetIntensity(light, intensity);

		_lights.push_back(light);
	}

	return true;
}

void Scene::clear() {
	for (ObjectID meshInstance : _meshInstances)
		RS::getSingleton().meshInstanceFree(meshInstance);

	for (ObjectID light : _lights)
		RS::getSingleton().lightFree(light);

	for (ObjectID mesh : _meshes)
		RS::getSingleton().meshFree(mesh);

	for (ObjectID material : _materials)
		RS::getSingleton().materialFree(material);

	for (ObjectID texture : _textures)
		RS::getSingleton().textureFree(texture);

	_meshInstances.clear();
	_lights.clear();
	_meshes.clear();
	_materials.clear();
	_textures.clear();
}
