#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H

#include <cstdint>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "../image.h"

#include "object_owner.h"

#include "types/allocated.h"
#include "types/camera.h"
#include "types/vertex.h"

#include "rendering_device.h"

#define NULL_HANDLE 0

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
		ObjectID material;
	};

	struct MaterialInfo {
		ObjectID albedo;
		ObjectID normal;
		ObjectID metallic;
		ObjectID roughness;
	};

private:
	struct PrimitiveRD {
		uint32_t indexCount;
		uint32_t firstIndex;
		ObjectID material;
	};

	struct MeshRD {
		AllocatedBuffer vertexBuffer;
		AllocatedBuffer indexBuffer;
		std::vector<PrimitiveRD> primitives;
	};

	struct MeshInstanceRD {
		glm::mat4 transform;
		ObjectID mesh;
	};

	struct MaterialRD {
		vk::DescriptorSet textureSet;
	};

	RenderingDevice *_pDevice;
	uint32_t _width, _height = 0;

	// fallbacks
	TextureRD _albedoFallback;
	TextureRD _normalFallback;
	TextureRD _metallicFallback;
	TextureRD _roughnessFallback;

	Camera _camera;
	ObjectOwner<MeshRD> _meshes;
	ObjectOwner<MeshInstanceRD> _meshInstances;
	ObjectOwner<TextureRD> _textures;
	ObjectOwner<MaterialRD> _materials;

public:
	RenderingServer(RenderingServer const &) = delete;
	void operator=(RenderingServer const &) = delete;

	void cameraSetTransform(const glm::mat4 &transform);
	void cameraSetFovY(float fovY);
	void cameraSetZNear(float zNear);
	void cameraSetZFar(float zFar);

	ObjectID meshCreate(const std::vector<Primitive> &primitives);
	void meshFree(ObjectID mesh);

	ObjectID meshInstanceCreate();
	void meshInstanceSetMesh(ObjectID meshInstance, ObjectID mesh);
	void meshInstanceSetTransform(ObjectID meshInstance, const glm::mat4 &transform);
	void meshInstanceFree(ObjectID meshInstance);

	ObjectID directionalLightCreate();
	void directionalLightSetDirection(ObjectID directionalLight, const glm::vec3 &direction);
	void directionalLightSetIntensity(ObjectID directionalLight, float intensity);
	void directionalLightSetColor(ObjectID directionalLight, const glm::vec3 &color);
	void directionalLightFree(ObjectID directionalLight);

	ObjectID pointLightCreate();
	void pointLightSetPosition(ObjectID pointLight, const glm::vec3 &position);
	void pointLightSetRange(ObjectID pointLight, float range);
	void pointLightSetColor(ObjectID pointLight, const glm::vec3 &color);
	void pointLightSetIntensity(ObjectID pointLight, float intensity);
	void pointLightFree(ObjectID pointLight);

	ObjectID textureCreate(const std::shared_ptr<Image> image);
	void textureFree(ObjectID texture);

	ObjectID materialCreate(const MaterialInfo &info);
	void materialFree(ObjectID material);

	void setExposure(float exposure);
	void setWhite(float white);

	void draw();

	vk::Instance getVkInstance() const;

	void windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void windowResized(uint32_t width, uint32_t height);

	void initImGui();

	void initialize(int argc, char **argv);
};

typedef RenderingServer RS;

#endif // !RENDERING_SERVER_H
