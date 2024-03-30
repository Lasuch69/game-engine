#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H

#include <cstdint>
#include <vector>

#include <SDL2/SDL_video.h>
#include <glm/glm.hpp>

#include "../rid_owner.h"

#include "rendering_device.h"
#include "types/vertex.h"

namespace RS {

typedef uint64_t Mesh;
typedef uint64_t MeshInstance;
typedef uint64_t Light;

enum Error {
	None = 0,
	InvalidResource = 1,
	InvalidParameter = 2,
	ResourceInUse = 3,
};

} // namespace RS

using namespace RS;

class RenderingServer {
private:
	struct CameraRS {
		// in radians
		float fovY = glm::radians(60.0f);
		float zNear = 0.1f;
		float zFar = 100.0f;

		glm::mat4 transform = glm::mat4(1.0f);

		glm::mat4 viewMatrix() const;
		glm::mat4 projectionMatrix(float aspect) const;
	};

	struct MeshRS {
		AllocatedBuffer vertexBuffer;
		AllocatedBuffer indexBuffer;
		uint32_t indexCount;
	};

	struct MeshInstanceRS {
		Mesh mesh;
		glm::mat4 transform;
	};

	struct LightRS {
		glm::vec3 position;
		float range;
		glm::vec3 color;
		float intensity;

		LightData toRaw() const;
	};

	SDL_Window *_pWindow;
	RenderingDevice *_pDevice;

	CameraRS _camera;
	RIDOwner<MeshRS> _meshes;
	RIDOwner<MeshInstanceRS> _meshInstances;
	RIDOwner<LightRS> _lights;

	void _updateLights();

public:
	Error cameraSetTransform(const glm::mat4 &transform);
	Error cameraSetFovY(float fovY);
	Error cameraSetZNear(float zNear);
	Error cameraSetZFar(float zFar);

	Mesh meshCreate(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);
	Error meshFree(Mesh mesh);

	Mesh meshInstanceCreate();
	Error meshInstanceSetMesh(MeshInstance meshInstance, Mesh mesh);
	Error meshInstanceSetTransform(MeshInstance meshInstance, const glm::mat4 &transform);
	Error meshInstanceFree(MeshInstance meshInstance);

	Light lightCreate();
	Error lightSetPosition(Light light, const glm::vec3 &position);
	Error lightSetRange(Light light, float range);
	Error lightSetColor(Light light, const glm::vec3 &color);
	Error lightSetIntensity(Light light, float intensity);
	Error lightFree(Light light);

	void draw();

	RenderingServer(SDL_Window *pWindow);
};

#endif // !RENDERING_SERVER_H
