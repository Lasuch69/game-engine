#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H

#include <cstdint>
#include <vector>

#include <SDL2/SDL_video.h>
#include <glm/glm.hpp>

#include "../rid_owner.h"

#include "types/camera.h"
#include "types/vertex.h"

#include "rendering_device.h"

typedef uint64_t MeshID;
typedef uint64_t MeshInstanceID;
typedef uint64_t PointLightID;

class RenderingServer {
private:
	struct Primitive {
		AllocatedBuffer vertexBuffer;
		AllocatedBuffer indexBuffer;
		uint32_t indexCount;
	};

	struct Mesh {
		std::vector<Primitive> primitives;
	};

	struct MeshInstance {
		MeshID mesh;
		glm::mat4 transform;
	};

	RenderingDevice *_pDevice;
	uint32_t _width, _height = 0;

	Camera _camera;
	RIDOwner<Mesh> _meshes;
	RIDOwner<MeshInstance> _meshInstances;
	RIDOwner<LightData> _pointLights;

	void _updateLights();

public:
	void cameraSetTransform(const glm::mat4 &transform);
	void cameraSetFovY(float fovY);
	void cameraSetZNear(float zNear);
	void cameraSetZFar(float zFar);

	MeshID meshCreate();
	void meshAddPrimitive(
			MeshID mesh, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
	void meshFree(MeshID mesh);

	MeshID meshInstanceCreate();
	void meshInstanceSetMesh(MeshInstanceID meshInstance, MeshID mesh);
	void meshInstanceSetTransform(MeshInstanceID meshInstance, const glm::mat4 &transform);
	void meshInstanceFree(MeshInstanceID meshInstance);

	PointLightID pointLightCreate();
	void pointLightSetPosition(PointLightID pointLight, const glm::vec3 &position);
	void pointLightSetRange(PointLightID pointLight, float range);
	void pointLightSetColor(PointLightID pointLight, const glm::vec3 &color);
	void pointLightSetIntensity(PointLightID pointLight, float intensity);
	void pointLightFree(PointLightID pointLight);

	void draw();

	void init(const std::vector<const char *> &extensions, bool validation = false);

	vk::Instance getVkInstance() const;

	void windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void windowResized(uint32_t width, uint32_t height);
};

typedef RenderingServer RS;

#endif // !RENDERING_SERVER_H
