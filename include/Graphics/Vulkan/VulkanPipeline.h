#pragma once
#include "Graphics/Vulkan/VulkanShader.h"
#include "VulkanDevice.h"

namespace PE::Graphics::Vulkan {
class VulkanPipeline {
public:
	VulkanPipeline()								  = default;
	VulkanPipeline(const VulkanPipeline &)			  = delete;
	VulkanPipeline &operator=(const VulkanPipeline &) = delete;
	VulkanPipeline(VulkanPipeline &&) noexcept;
	VulkanPipeline &operator=(VulkanPipeline &&) noexcept;
	~VulkanPipeline() = default;

	ERROR_CODE Initialize(VulkanDevice *device, const VulkanShader &shader, VkPipelineLayout layout, VkExtent2D extent,
						  const PipelineDescription &desc);
	ERROR_CODE Initialize(VulkanDevice *device, const VulkanShader &shader, VkPipelineLayout layout, VkExtent2D extent,
						  const PipelineDescription &desc, const std::vector<VkVertexInputBindingDescription> &bindings,
						  const std::vector<VkVertexInputAttributeDescription> &attributes);

	void Shutdown();

	void Bind(VkCommandBuffer cmd);

	[[nodiscard]] VkPipelineLayout GetLayout() const { return m_layout; }
	[[nodiscard]] VkPipeline	   GetHandle() const { return m_vkPipeline; }

private:
	VulkanDevice *ref_device = nullptr;

	SystemState		 m_state	  = SystemState::Uninitialized;
	VkPipeline		 m_vkPipeline = VK_NULL_HANDLE;
	VkPipelineLayout m_layout	  = VK_NULL_HANDLE;
};
}  // namespace PE::Graphics::Vulkan