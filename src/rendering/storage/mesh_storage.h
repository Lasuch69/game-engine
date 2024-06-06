#ifndef MESH_STORAGE_H
#define MESH_STORAGE_H

#include <cstdint>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include <rendering/object_owner.h>
#include <rendering/types/allocated.h>

typedef struct {
	uint32_t firstIndex;
	uint32_t indexCount;
	ObjectID material;
} PrimitiveRD;

typedef struct {
	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;

	PrimitiveRD *pPrimitives;
	uint32_t primitiveCount;
} MeshRD;

class MeshStorage {
private:
	ObjectOwner<MeshRD> _meshOwner;

public:
	inline ObjectID meshAppend(const MeshRD &mesh) {
		return _meshOwner.append(mesh);
	}

	inline MeshRD meshRemove(ObjectID meshID) {
		return _meshOwner.remove(meshID);
	}
};

#endif // !MESH_STORAGE_H
