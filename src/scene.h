#ifndef SCENE_H
#define SCENE_H

#include <filesystem>
#include <vector>

#include "rendering/rendering_server.h"

class Scene {
private:
	std::vector<Texture> _textures;
	std::vector<Material> _materials;

	std::vector<Mesh> _meshes;
	std::vector<MeshInstance> _meshInstances;

	std::vector<PointLight> _pointLights;

public:
	bool load(const std::filesystem::path &path, RenderingServer *pRenderingServer);
	void clear(RenderingServer *pRenderingServer);
};

#endif // !SCENE_H
