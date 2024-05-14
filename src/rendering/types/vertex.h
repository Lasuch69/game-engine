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
	glm::vec3 tangent;
	glm::vec2 uv;

	static vk::VertexInputBindingDescription getBindingDescription() {
		vk::VertexInputBindingDescription bindingDescription;
		bindingDescription.setBinding(0);
		bindingDescription.setStride(sizeof(Vertex));
		bindingDescription.setInputRate(vk::VertexInputRate::eVertex);

		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
		std::array<vk::VertexInputAttributeDescription, 4> attributeDescriptions;

		// Position
		attributeDescriptions[0].setLocation(0);
		attributeDescriptions[0].setBinding(0);
		attributeDescriptions[0].setFormat(vk::Format::eR32G32B32Sfloat);
		attributeDescriptions[0].setOffset(offsetof(Vertex, position));

		// Normal
		attributeDescriptions[1].setLocation(1);
		attributeDescriptions[1].setBinding(0);
		attributeDescriptions[1].setFormat(vk::Format::eR32G32B32Sfloat);
		attributeDescriptions[1].setOffset(offsetof(Vertex, normal));

		// Tangent
		attributeDescriptions[2].setLocation(2);
		attributeDescriptions[2].setBinding(0);
		attributeDescriptions[2].setFormat(vk::Format::eR32G32B32Sfloat);
		attributeDescriptions[2].setOffset(offsetof(Vertex, tangent));

		// TexCoord
		attributeDescriptions[3].setLocation(3);
		attributeDescriptions[3].setBinding(0);
		attributeDescriptions[3].setFormat(vk::Format::eR32G32Sfloat);
		attributeDescriptions[3].setOffset(offsetof(Vertex, uv));

		return attributeDescriptions;
	}

	bool operator==(const Vertex &v) const {
		return position == v.position && normal == v.normal && tangent == v.tangent && uv == v.uv;
	}
};

namespace std {
template <> struct hash<Vertex> {
	size_t operator()(Vertex const &v) const {
		return ((hash<glm::vec3>()(v.position) ^ (hash<glm::vec3>()(v.normal) << 1) ^
						(hash<glm::vec3>()(v.tangent) << 1)) >>
					   1) ^
			   (hash<glm::vec2>()(v.uv) << 1);
	}
};
} // namespace std

#endif // !VERTEX_H
