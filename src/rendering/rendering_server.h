#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H

#include "rendering/material.h"
#include <cstdint>

#include <glm/glm.hpp>

#include <rendering/rendering_device.h>
#include <rendering/storage/light_storage.h>
#include <rendering/storage/mesh_storage.h>
#include <rendering/storage/texture_storage.h>
#include <rendering/types/camera.h>

#include <io/mesh.h>

class Image;
struct SDL_Window;

class RenderingServer {
public:
	static RenderingServer &getSingleton() {
		static RenderingServer instance;
		return instance;
	}

private:
	RenderingDevice _renderingDevice;

	Camera _camera;

	LightStorage _lightStorage;
	MeshStorage _meshStorage;
	TextureStorage _textureStorage;

	// TODO: move this somewhere else
	DepthMaterial _depthMaterial;
	StandardMaterial _standardMaterial;
	SkyEffect _skyEffect;
	TonemapEffect _tonemapEffect;

	ObjectID _meshCreate(const Mesh &mesh);
	void _meshDestroy(ObjectID meshID);

	ObjectID _textureCreate(const Image *pImage);
	void _textureDestroy(ObjectID textureID);

	RenderingServer() {}

public:
	RenderingServer(RenderingServer const &) = delete;
	void operator=(RenderingServer const &) = delete;

	void cameraSetTransform(const glm::mat4 &transform);
	void cameraSetFovY(float fovY);
	void cameraSetZNear(float zNear);
	void cameraSetZFar(float zFar);

	ObjectID meshCreate(const Mesh &mesh);
	void meshFree(ObjectID mesh);

	ObjectID meshInstanceCreate();
	void meshInstanceSetMesh(ObjectID meshInstance, ObjectID mesh);
	void meshInstanceSetTransform(ObjectID meshInstance, const glm::mat4 &transform);
	void meshInstanceFree(ObjectID meshInstance);

	ObjectID lightCreate(LightType type);
	void lightSetTransform(ObjectID light, const glm::mat4 &transform);
	void lightSetColor(ObjectID light, const glm::vec3 &color);
	void lightSetIntensity(ObjectID light, float intensity);
	void lightFree(ObjectID light);

	ObjectID textureCreate(const Image *pImage);
	void textureDestroy(ObjectID texture);

	void draw();

	void windowInit(SDL_Window *pWindow);
	void windowResized(uint32_t width, uint32_t height);

	void initialize(int argc, char **argv);
};

typedef RenderingServer RS;

#endif // !RENDERING_SERVER_H
