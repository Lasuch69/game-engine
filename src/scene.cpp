#include "scene.h"

#include "loader.h"

bool Scene::load(std::filesystem::path path, RenderingServer *pRenderingServer) {
	Loader::SceneGlTF *pScene = Loader::loadGltf(path);

	if (pScene == nullptr) {
		return false;
	}

	for (Image *pImage : pScene->images) {
		Texture texture = pRenderingServer->textureCreate(pImage);
		_textures.push_back(texture);

		free(pImage);
	}

	for (const Loader::Material &_material : pScene->materials) {
		Texture albedo = _textures[_material.albedoIndex];
		_materials.push_back(pRenderingServer->materialCreate(albedo));
	}

	for (const Loader::Camera &_camera : pScene->cameras) {
		pRenderingServer->cameraSetTransform(_camera.transform);
		pRenderingServer->cameraSetFovY(_camera.fovY);
		pRenderingServer->cameraSetZNear(_camera.zNear);
		pRenderingServer->cameraSetZFar(_camera.zFar);
	}

	for (const Loader::MeshInstance &_meshInstance : pScene->meshInstances) {
		Loader::Mesh _mesh = pScene->meshes[_meshInstance.meshIndex];
		Mesh mesh = pRenderingServer->meshCreate();

		for (const Loader::Primitive &primitive : _mesh.primitives) {
			pRenderingServer->meshAddPrimitive(mesh, primitive.vertices, primitive.indices,
					_materials[primitive.materialIndex]);
		}

		MeshInstance meshInstance = pRenderingServer->meshInstanceCreate();
		pRenderingServer->meshInstanceSetMesh(meshInstance, mesh);
		pRenderingServer->meshInstanceSetTransform(meshInstance, _meshInstance.transform);

		_meshes.push_back(mesh);
		_meshInstances.push_back(meshInstance);
	}

	for (const Loader::PointLight &_pointLight : pScene->pointLights) {
		PointLight pointLight = pRenderingServer->pointLightCreate();
		pRenderingServer->pointLightSetPosition(pointLight, _pointLight.position);
		pRenderingServer->pointLightSetRange(pointLight, _pointLight.range);
		pRenderingServer->pointLightSetColor(pointLight, _pointLight.color);
		pRenderingServer->pointLightSetIntensity(pointLight, _pointLight.intensity);

		_pointLights.push_back(pointLight);
	}

	free(pScene);
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
