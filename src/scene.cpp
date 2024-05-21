#include <cstdint>
#include <filesystem>
#include <optional>

#include "rendering/rendering_server.h"

#include "loader.h"
#include "scene.h"

bool Scene::load(const std::filesystem::path &path) {
	Loader::Scene scene = Loader::loadGltf(path);

	for (const Loader::Material &sceneMaterial : scene.materials) {
		RS::MaterialInfo info;

		std::optional<size_t> albedoIndex = sceneMaterial.albedoIndex;
		if (albedoIndex.has_value()) {
			std::shared_ptr<Image> albedoMap = scene.images[albedoIndex.value()];

			ObjectID t = RS::getInstance().textureCreate(albedoMap);
			_textures.push_back(t);

			info.albedo = t;
		}

		std::optional<size_t> normalIndex = sceneMaterial.normalIndex;
		if (normalIndex.has_value()) {
			std::shared_ptr<Image> normalMap = scene.images[normalIndex.value()];

			ObjectID t = RS::getInstance().textureCreate(normalMap);
			_textures.push_back(t);

			info.normal = t;
		}

		std::optional<size_t> metallicIndex = sceneMaterial.metallicIndex;
		if (metallicIndex.has_value()) {
			std::shared_ptr<Image> metallicMap = scene.images[metallicIndex.value()];

			ObjectID t = RS::getInstance().textureCreate(metallicMap);
			_textures.push_back(t);

			info.metallic = t;
		}

		std::optional<size_t> roughnessIndex = sceneMaterial.roughnessIndex;
		if (roughnessIndex.has_value()) {
			std::shared_ptr<Image> roughnessMap = scene.images[roughnessIndex.value()];

			ObjectID t = RS::getInstance().textureCreate(roughnessMap);
			_textures.push_back(t);

			info.roughness = t;
		}

		ObjectID material = RS::getInstance().materialCreate(info);
		_materials.push_back(material);
	}

	for (const Loader::Mesh &sceneMesh : scene.meshes) {
		std::vector<RS::Primitive> primitives;
		for (const Loader::Primitive &meshPrimitive : sceneMesh.primitives) {
			uint64_t materialIndex = meshPrimitive.materialIndex;
			ObjectID material = _materials[materialIndex];

			RS::Primitive primitive = {};
			primitive.vertices = meshPrimitive.vertices;
			primitive.indices = meshPrimitive.indices;
			primitive.material = material;

			primitives.push_back(primitive);
		}

		ObjectID mesh = RS::getInstance().meshCreate(primitives);
		_meshes.push_back(mesh);
	}

	for (const Loader::MeshInstance &sceneMeshInstance : scene.meshInstances) {
		uint64_t meshIndex = sceneMeshInstance.meshIndex;

		ObjectID mesh = _meshes[meshIndex];
		glm::mat4 transform = sceneMeshInstance.transform;

		ObjectID meshInstance = RS::getInstance().meshInstanceCreate();
		RS::getInstance().meshInstanceSetMesh(meshInstance, mesh);
		RS::getInstance().meshInstanceSetTransform(meshInstance, transform);

		_meshInstances.push_back(meshInstance);
	}

	for (const Loader::Light &sceneLight : scene.lights) {
		glm::mat4 transform = sceneLight.transform;
		float range = sceneLight.range.value_or(0.0f);
		glm::vec3 color = sceneLight.color;
		float intensity = sceneLight.intensity;

		ObjectID light;

		switch (sceneLight.type) {
			case Loader::LightType::Directional:
				light = RS::getInstance().lightCreate(LightType::Directional);
				break;
			case Loader::LightType::Point:
				light = RS::getInstance().lightCreate(LightType::Point);
				break;
		}

		RS::getInstance().lightSetTransform(light, transform);
		RS::getInstance().lightSetRange(light, range);
		RS::getInstance().lightSetColor(light, color);
		RS::getInstance().lightSetIntensity(light, intensity);

		_lights.push_back(light);
	}

	return true;
}

void Scene::clear() {
	for (ObjectID meshInstance : _meshInstances)
		RS::getInstance().meshInstanceFree(meshInstance);

	for (ObjectID light : _lights)
		RS::getInstance().lightFree(light);

	for (ObjectID mesh : _meshes)
		RS::getInstance().meshFree(mesh);

	for (ObjectID material : _materials)
		RS::getInstance().materialFree(material);

	for (ObjectID texture : _textures)
		RS::getInstance().textureFree(texture);

	_meshInstances.clear();
	_lights.clear();
	_meshes.clear();
	_materials.clear();
	_textures.clear();
}
