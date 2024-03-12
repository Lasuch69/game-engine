#ifndef VERTEX_H
#define VERTEX_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.hpp>

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription() {
		return { 0, sizeof(Vertex) };
	}

	static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
		return {
			vk::VertexInputAttributeDescription(
					0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
			vk::VertexInputAttributeDescription(
					1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
			vk::VertexInputAttributeDescription(
					2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
			vk::VertexInputAttributeDescription(
					3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
		};
	}

	bool operator==(const Vertex &other) const {
		return position == other.position && normal == other.normal && color == other.color &&
				texCoord == other.texCoord;
	}
};

namespace std {
template <> struct hash<Vertex> {
	size_t operator()(Vertex const &vertex) const {
		return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1) ^
						(hash<glm::vec3>()(vertex.color) << 1)) >>
					   1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};
} // namespace std

#endif // !VERTEX_H
