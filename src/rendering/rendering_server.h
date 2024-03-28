#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H

#include <cstdint>
#include <vector>

#include <SDL2/SDL_video.h>
#include <glm/glm.hpp>

#include "../rid_owner.h"

#include "camera.h"
#include "rendering_device.h"

typedef uint64_t MeshID;
typedef uint64_t MeshInstanceID;
typedef uint64_t LightID;

struct MeshInstance {
	MeshID mesh;
	glm::mat4 transform;
};

class RenderingServer {
private:
	SDL_Window *_pWindow;
	RenderingDevice *_pDevice;

	Camera _camera;
	RIDOwner<Mesh> _meshes;
	RIDOwner<MeshInstance> _meshInstances;
	RIDOwner<LightData> _lights;

	void _updateLights();

public:
	void cameraSetTransform(const glm::mat4 &transform);
	void cameraSetFovY(float fovY);
	void cameraSetZNear(float zNear);
	void cameraSetZFar(float zFar);

	MeshID meshCreate(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
	void meshFree(MeshID meshID);

	MeshID meshInstanceCreate();
	void meshInstanceSetMesh(MeshInstanceID meshInstanceID, MeshID meshID);
	void meshInstanceSetTransform(MeshInstanceID meshInstanceID, const glm::mat4 &transform);
	void meshInstanceFree(MeshInstanceID meshInstanceID);

	LightID lightCreate();
	void lightSetPosition(LightID lightID, const glm::vec3 &position);
	void lightSetRange(LightID lightID, float range);
	void lightSetColor(LightID lightID, const glm::vec3 &color);
	void lightSetIntensity(LightID lightID, float intensity);
	void lightFree(LightID lightID);

	void draw();

	RenderingServer(SDL_Window *pWindow);
};

typedef RenderingServer RS;

#endif // !RENDERING_SERVER_H
