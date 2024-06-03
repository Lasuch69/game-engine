#ifndef RENDERING_SERVER_H
#define RENDERING_SERVER_H

#include <cstdint>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "object_owner.h"
#include "storage/light_storage.h"

#include "types/camera.h"
#include "types/resource.h"
#include "types/vertex.h"

#define NULL_HANDLE 0

class Image;

class RenderingServer {
public:
	static RenderingServer &getSingleton() {
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

	ObjectID lightCreate(LightType type);
	void lightSetTransform(ObjectID light, const glm::mat4 &transform);
	void lightSetRange(ObjectID light, float range);
	void lightSetColor(ObjectID light, const glm::vec3 &color);
	void lightSetIntensity(ObjectID light, float intensity);
	void lightFree(ObjectID light);

	ObjectID textureCreate(const std::shared_ptr<Image> image);
	void textureFree(ObjectID texture);

	ObjectID materialCreate(const MaterialInfo &info);
	void materialFree(ObjectID material);

	void setExposure(float exposure);
	void setWhite(float white);

	void environmentSkyUpdate(const std::shared_ptr<Image> image);

	void draw();

	vk::Instance getVkInstance() const;

	void windowInit(vk::SurfaceKHR surface, uint32_t width, uint32_t height);
	void windowResized(uint32_t width, uint32_t height);

	void initialize(int argc, char **argv);
};

typedef RenderingServer RS;

#endif // !RENDERING_SERVER_H
