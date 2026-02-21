#pragma once
#include <array>

#include "Math/Math.h"

#ifdef PE_VULKAN
#include <vulkan/vulkan_core.h>
#endif

namespace PE::Graphics {
struct Vertex {
	Vertex();
	Vertex(Math::Vector3 position, Math::Vector3 normal, Math::Vector3 tangent, Math::Vector2 texcoord);
	Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v);

	Math::Vector3 Position{};
	Math::Vector3 Normal{};
	Math::Vector3 Tangent{};
	Math::Vector2 TexC{};

#ifdef PE_VULKAN
	static VkVertexInputBindingDescription GetBindingDescription() {
		constexpr VkVertexInputBindingDescription bindingDescription{
			.binding   = 0,
			.stride	   = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format	  = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset	  = offsetof(Vertex, Position);

		attributeDescriptions[1].binding  = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format	  = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset	  = offsetof(Vertex, Normal);

		attributeDescriptions[2].binding  = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format	  = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset	  = offsetof(Vertex, Tangent);

		attributeDescriptions[3].binding  = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format	  = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset	  = offsetof(Vertex, TexC);

		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptionForParticles() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format	  = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset	  = offsetof(Vertex, Position);

		attributeDescriptions[1].binding  = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format	  = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset	  = offsetof(Vertex, TexC);

		return attributeDescriptions;
	}
#endif
};
}  // namespace PE::Graphics