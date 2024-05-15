#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H

#include <cstdint>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "../image.h"
#include "../rid_owner.h"

#include "types/allocated.h"
#include "types/camera.h"
#include "types/vertex.h"

#include "rendering_device.h"

typedef uint64_t Mesh;
typedef uint64_t MeshInstance;
typedef uint64_t Texture;
typedef uint64_t Material;

#define NULL_HANDLE 0

enum MSAA {
	MSAA_X1 = 0,
	MSAA_X2 = 1,
	MSAA_X4 = 2,
	MSAA_X8 = 3,
	MSAA_MAX = 4,
};

class RenderingServer {
public:
	static RenderingServer &getInstance() {
		static RenderingServer instance;
		return instance;
	}

private:
	RenderingServer() {}

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
		vk::DescriptorSet textureSet;
	};

	RenderingDevice *_pDevice;
	uint32_t _width, _height = 0;

	Texture _whiteTexture = NULL_HANDLE;

	Camera _camera;
	RIDOwner<MeshRD> _meshes;
	RIDOwner<MeshInstanceRD> _meshInstances;
	RIDOwner<TextureRD> _textures;
	RIDOwner<MaterialRD> _materials;

public:
	RenderingServer(RenderingServer const &) = delete;
	void operator=(RenderingServer const &) = delete;

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

	DirectionalLight directionalLightCreate();
	void directionalLightSetDirection(
			DirectionalLight directionalLight, const glm::vec3 &direction);
	void directionalLightSetIntensity(DirectionalLight directionalLight, float intensity);
	void directionalLightSetColor(DirectionalLight directionalLight, const glm::vec3 &color);
	void directionalLightFree(DirectionalLight directionalLight);

	PointLight pointLightCreate();
	void pointLightSetPosition(PointLight pointLight, const glm::vec3 &position);
	void pointLightSetRange(PointLight pointLight, float range);
	void pointLightSetColor(PointLight pointLight, const glm::vec3 &color);
	void pointLightSetIntensity(PointLight pointLight, float intensity);
	void pointLightFree(PointLight pointLight);

	Texture textureCreate(const std::shared_ptr<Image> image);
	void textureFree(Texture texture);

	Material materialCreate(Texture albedo = NULL_HANDLE, Texture normal = NULL_HANDLE,
			Texture metallic = NULL_HANDLE, Texture roughness = NULL_HANDLE);
	void materialFree(Material material);

	void MSAASamplesSet(MSAA samples);
	MSAA MSAASamplesGet() const;

	void draw();

	vk::Instance getVkInstance() const;

	void windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void windowResized(uint32_t width, uint32_t height);

	void initImGui();

	void initialize(int argc, char **argv);
};

typedef RenderingServer RS;

#endif // !RENDERING_SERVER_H
