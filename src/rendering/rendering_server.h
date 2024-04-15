#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H

#include <cstdint>
#include <vector>

#include <SDL2/SDL_video.h>
#include <glm/glm.hpp>

#include "../image.h"
#include "../rid_owner.h"

#include "types/allocated.h"
#include "types/camera.h"
#include "types/vertex.h"

#include "rendering_device.h"

typedef uint64_t Mesh;
typedef uint64_t MeshInstance;
typedef uint64_t PointLight;
typedef uint64_t Texture;
typedef uint64_t Material;

#define NULL_HANDLE 0

class RenderingServer {
public:
	struct Primitive {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		Material material;
	};

private:
	struct PrimitiveRD {
		uint32_t indexCount;
		uint32_t firstIndex;
		Material material;
	};

	struct MeshRD {
		AllocatedBuffer vertexBuffer;
		AllocatedBuffer indexBuffer;
		std::vector<PrimitiveRD> primitives;
	};

	struct MeshInstanceRD {
		glm::mat4 transform;
		Mesh mesh;
	};

	struct MaterialRD {
		vk::DescriptorSet albedoSet;
	};

	bool _useValidation = false;
	RenderingDevice *_pDevice;
	uint32_t _width, _height = 0;

	Texture _whiteTexture = NULL_HANDLE;

	Camera _camera;
	RIDOwner<MeshRD> _meshes;
	RIDOwner<MeshInstanceRD> _meshInstances;
	RIDOwner<PointLightRD> _pointLights;
	RIDOwner<TextureRD> _textures;
	RIDOwner<MaterialRD> _materials;

	void _updateLights();

public:
	void cameraSetTransform(const glm::mat4 &transform);
	void cameraSetFovY(float fovY);
	void cameraSetZNear(float zNear);
	void cameraSetZFar(float zFar);

	Mesh meshCreate(const std::vector<Primitive> &primitives);
	void meshFree(Mesh mesh);

	Mesh meshInstanceCreate();
	void meshInstanceSetMesh(MeshInstance meshInstance, Mesh mesh);
	void meshInstanceSetTransform(MeshInstance meshInstance, const glm::mat4 &transform);
	void meshInstanceFree(MeshInstance meshInstance);

	PointLight pointLightCreate();
	void pointLightSetPosition(PointLight pointLight, const glm::vec3 &position);
	void pointLightSetRange(PointLight pointLight, float range);
	void pointLightSetColor(PointLight pointLight, const glm::vec3 &color);
	void pointLightSetIntensity(PointLight pointLight, float intensity);
	void pointLightFree(PointLight pointLight);

	Texture textureCreate(Image *pImage);
	void textureFree(Texture texture);

	Material materialCreate(Texture albedo = NULL_HANDLE);
	void materialFree(Material material);

	void draw();

	void init(const std::vector<const char *> &extensions);

	vk::Instance getVkInstance() const;

	void windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void windowResized(uint32_t width, uint32_t height);

	RenderingServer(int argc, char **argv);
};

typedef RenderingServer RS;

#endif // !RENDERING_SERVER_H
