#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H

#include <cstdint>
#include <vector>

#include <SDL2/SDL_video.h>
#include <glm/glm.hpp>

#include "../rid_owner.h"

#include "camera.h"
#include "rendering_device.h"
#include "vertex.h"

typedef uint64_t MeshID;
typedef uint64_t LightID;

class RenderingServer {
private:
	SDL_Window *_pWindow;
	RenderingDevice *_pDevice;

	Camera _camera;
	RIDOwner<Mesh> _meshes;
	RIDOwner<LightData> _lights;

	std::vector<std::tuple<MeshID, glm::mat4>> _drawQueue;

	void _updateLights();

public:
	void cameraSetTransform(const glm::mat4 &transform);
	void cameraSetFovY(float fovY);
	void cameraSetZNear(float zNear);
	void cameraSetZFar(float zFar);

	MeshID meshCreate(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
	void meshFree(MeshID meshID);

	LightID lightCreate();
	void lightSetPosition(LightID lightID, const glm::vec3 &position);
	void lightSetRange(LightID lightID, float range);
	void lightSetColor(LightID lightID, const glm::vec3 &color);
	void lightSetIntensity(LightID lightID, float intensity);
	void lightFree(LightID lightID);

	void drawMesh(MeshID meshID, const glm::mat4 &transform);
	void submit();

	RenderingServer(SDL_Window *pWindow);
};

typedef RenderingServer RS;

#endif // !RENDERING_SERVER_H
