#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <filesystem>

class AssetLoader {
public:
	static void loadScene(const std::filesystem::path &file);
};

#endif // !ASSET_LOADER_H
