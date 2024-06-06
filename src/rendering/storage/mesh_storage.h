#ifndef MESH_STORAGE_H
#define MESH_STORAGE_H

#include <cstdint>

#include <vector>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include <rendering/object_owner.h>
#include <rendering/types/allocated.h>

typedef struct {
	uint32_t firstIndex;
	uint32_t indexCount;
	ObjectID materialID;
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

	inline const MeshRD *meshGet(ObjectID meshID) {
		if (!_meshOwner.has(meshID))
			return nullptr;

		return &_meshOwner[meshID];
	}

	inline std::vector<MeshRD> meshElements() const {
		const std::unordered_map<ObjectID, MeshRD> &map = _meshOwner.map();
		std::vector<MeshRD> elements = {};

		for (auto it = map.begin(); it != map.end(); ++it)
			elements.push_back(it->second);

		return elements;
	}

	inline MeshRD meshRemove(ObjectID meshID) {
		return _meshOwner.remove(meshID);
	}
};

#endif // !MESH_STORAGE_H
