#ifndef SCENE_H
#define SCENE_H

#include <cstdint>
#include <filesystem>
#include <vector>

typedef uint64_t ObjectID;

class Scene {
private:
	std::vector<ObjectID> _textures;
	std::vector<ObjectID> _materials;

	std::vector<ObjectID> _meshes;
	std::vector<ObjectID> _meshInstances;

	std::vector<ObjectID> _directionalLights;
	std::vector<ObjectID> _pointLights;

public:
	bool load(const std::filesystem::path &path);
	void clear();
};

#endif // !SCENE_H
