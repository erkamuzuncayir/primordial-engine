#pragma once
#include <vulkan/vulkan_core.h>

#include <utility>

#include "Graphics/RenderTypes.h"

namespace PE::Graphics::Vulkan {
struct PipelineDescription {
	ShaderID shaderID = INVALID_HANDLE;

	bool			wireframe		 = false;
	VkCullModeFlags cullMode		 = VK_CULL_MODE_BACK_BIT;
	bool			enableDepthTest	 = true;
	bool			enableDepthWrite = true;
	bool			enableDepthBias	 = false;
	bool			enableBlend		 = true;
	VkCompareOp		compareOp		 = VK_COMPARE_OP_LESS;

	VkFormat colorFormat = VK_FORMAT_UNDEFINED;
	VkFormat depthFormat = VK_FORMAT_UNDEFINED;

	bool operator==(const PipelineDescription &other) const = default;

	struct PipelineDescriptionHash {
		std::size_t operator()(const PipelineDescription &desc) const {
			std::size_t seed = 0;

			auto hash_combine = [&seed](const size_t value) { seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2); };

			hash_combine(std::hash<uint32_t>{}(desc.shaderID));
			hash_combine(std::hash<bool>{}(desc.wireframe));
			hash_combine(std::hash<VkCullModeFlags>{}(desc.cullMode));
			hash_combine(std::hash<bool>{}(desc.enableDepthTest));
			hash_combine(std::hash<bool>{}(desc.enableDepthWrite));
			hash_combine(std::hash<bool>{}(desc.enableDepthBias));
			hash_combine(std::hash<bool>{}(desc.enableBlend));
			hash_combine(std::hash<uint32_t>{}(static_cast<uint32_t>(desc.compareOp)));
			hash_combine(std::hash<uint32_t>{}(static_cast<uint32_t>(desc.colorFormat)));
			hash_combine(std::hash<uint32_t>{}(static_cast<uint32_t>(desc.depthFormat)));

			return seed;
		}
	};
};

struct VulkanTextureWrapper {
	VkImage		   image	 = VK_NULL_HANDLE;
	VkImageView	   imageView = VK_NULL_HANDLE;
	VkDeviceMemory memory	 = VK_NULL_HANDLE;

	uint32_t width		= 0;
	uint32_t height		= 0;
	uint32_t depth		= 1;
	uint32_t mipLevels	= 1;
	uint32_t layerCount = 1;

	VkFormat			  format  = VK_FORMAT_UNDEFINED;
	VkImageUsageFlags	  usage	  = 0;
	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	VkImageCreateFlags	  flags	  = 0;

	VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VulkanTextureWrapper() = default;

	VulkanTextureWrapper(const VulkanTextureWrapper &)			  = delete;
	VulkanTextureWrapper &operator=(const VulkanTextureWrapper &) = delete;

	VulkanTextureWrapper(VulkanTextureWrapper &&other) noexcept { *this = std::move(other); }

	VulkanTextureWrapper &operator=(VulkanTextureWrapper &&other) noexcept {
		if (this != &other) {
			image	   = std::exchange(other.image, VK_NULL_HANDLE);
			imageView  = std::exchange(other.imageView, VK_NULL_HANDLE);
			memory	   = std::exchange(other.memory, VK_NULL_HANDLE);
			width	   = std::exchange(other.width, 0);
			height	   = std::exchange(other.height, 0);
			depth	   = std::exchange(other.depth, 1);
			mipLevels  = std::exchange(other.mipLevels, 1);
			layerCount = std::exchange(other.layerCount, 1);

			format	= std::exchange(other.format, VK_FORMAT_UNDEFINED);
			usage	= std::exchange(other.usage, 0);
			samples = std::exchange(other.samples, VK_SAMPLE_COUNT_1_BIT);
			flags	= std::exchange(other.flags, 0);

			currentLayout = std::exchange(other.currentLayout, VK_IMAGE_LAYOUT_UNDEFINED);
		}
		return *this;
	}
};

struct VulkanMeshWrapper {
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VkBuffer indexBuffer  = VK_NULL_HANDLE;
	uint32_t indexCount	  = 0;
	uint32_t vertexCount  = 0;
	uint32_t firstVertex  = 0;
	uint32_t firstIndex	  = 0;

	VulkanMeshWrapper() = default;

	VulkanMeshWrapper(const VulkanMeshWrapper &)			= delete;
	VulkanMeshWrapper &operator=(const VulkanMeshWrapper &) = delete;

	VulkanMeshWrapper(VulkanMeshWrapper &&other) noexcept { *this = std::move(other); }

	VulkanMeshWrapper &operator=(VulkanMeshWrapper &&other) noexcept {
		if (this != &other) {
			vertexBuffer = std::exchange(other.vertexBuffer, VK_NULL_HANDLE);
			indexBuffer	 = std::exchange(other.indexBuffer, VK_NULL_HANDLE);
			indexCount	 = std::exchange(other.indexCount, 0);
			vertexCount	 = std::exchange(other.vertexCount, 0);
			firstVertex	 = std::exchange(other.firstVertex, 0);
			firstIndex	 = std::exchange(other.firstIndex, 0);
		}
		return *this;
	}
};

struct VulkanRenderTargetWrapper {
	VkImage		   image	 = VK_NULL_HANDLE;
	VkImageView	   imageView = VK_NULL_HANDLE;
	VkDeviceMemory memory	 = VK_NULL_HANDLE;
	VkFormat	   format	 = VK_FORMAT_UNDEFINED;
	TextureID	   textureID = 0;

	VulkanRenderTargetWrapper() = default;

	VulkanRenderTargetWrapper(const VulkanRenderTargetWrapper &)			= delete;
	VulkanRenderTargetWrapper &operator=(const VulkanRenderTargetWrapper &) = delete;

	VulkanRenderTargetWrapper(VulkanRenderTargetWrapper &&other) noexcept { *this = std::move(other); }

	VulkanRenderTargetWrapper &operator=(VulkanRenderTargetWrapper &&other) noexcept {
		if (this != &other) {
			image	  = std::exchange(other.image, VK_NULL_HANDLE);
			imageView = std::exchange(other.imageView, VK_NULL_HANDLE);
			memory	  = std::exchange(other.memory, VK_NULL_HANDLE);
			format	  = std::exchange(other.format, VK_FORMAT_UNDEFINED);
			textureID = std::exchange(other.textureID, 0);
		}
		return *this;
	}
};

struct ShadowMapResources {
	VulkanTextureWrapper texture;
	VkSampler			 sampler = VK_NULL_HANDLE;
	const uint32_t		 dim	 = 4096;
};
}  // namespace PE::Graphics::Vulkan