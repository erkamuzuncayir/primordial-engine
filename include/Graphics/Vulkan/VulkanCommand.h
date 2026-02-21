#pragma once
#include <vulkan/vulkan.h>

#include <vector>

#include "Common/Common.h"

namespace PE::Graphics::Vulkan {
class VulkanDevice;

class VulkanCommand {
public:
	VulkanCommand()									= default;
	VulkanCommand(const VulkanCommand &)			= delete;
	VulkanCommand &operator=(const VulkanCommand &) = delete;
	VulkanCommand(VulkanCommand &&)					= delete;
	VulkanCommand &operator=(VulkanCommand &&)		= delete;
	~VulkanCommand()								= default;
	ERROR_CODE Initialize(VulkanDevice *device, uint32_t framesInFlight);
	void	   Shutdown();

	ERROR_CODE CreateCommandPool();
	ERROR_CODE CreateCommandBuffers(uint32_t frames);

	[[nodiscard]] VkCommandPool	  GetCommandPool() const { return m_commandPool; }
	[[nodiscard]] VkCommandBuffer GetCommandBuffer(uint32_t index) const { return m_commandBuffers[index]; }

private:
	SystemState					 m_state	   = SystemState::Uninitialized;
	VulkanDevice				*ref_device	   = nullptr;
	VkCommandPool				 m_commandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> m_commandBuffers;
};
}  // namespace PE::Graphics::Vulkan