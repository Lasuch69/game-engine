#ifndef TEXTURE_STORAGE_H
#define TEXTURE_STORAGE_H

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_handles.hpp>

#include <rendering/object_owner.h>
#include <rendering/types/allocated.h>

typedef struct {
	AllocatedImage image;
	vk::ImageView imageView;
	vk::Sampler sampler;
} TextureRD;

class TextureStorage {
private:
	ObjectOwner<TextureRD> _textureOwner;

public:
	inline ObjectID textureAppend(const TextureRD &texture) {
		return _textureOwner.append(texture);
	}

	inline TextureRD textureRemove(ObjectID textureID) {
		return _textureOwner.remove(textureID);
	}
};

#endif // !TEXTURE_STORAGE_H
