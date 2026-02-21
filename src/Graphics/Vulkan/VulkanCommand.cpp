#include "Graphics/Vulkan/VulkanCommand.h"

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Utilities/Logger.h"

namespace PE::Graphics::Vulkan {
ERROR_CODE VulkanCommand::Initialize(VulkanDevice *device, uint32_t framesInFlight) {
	PE_CHECK_STATE_INIT(m_state, "This vulkan command is already initialized.");
	m_state = SystemState::Initializing;

	ref_device = device;
	ERROR_CODE result;
	PE_CHECK(result, CreateCommandPool());
	PE_CHECK(result, CreateCommandBuffers(framesInFlight));

	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

void VulkanCommand::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return;
	m_state = SystemState::ShuttingDown;
	if (m_commandPool != VK_NULL_HANDLE) vkDestroyCommandPool(ref_device->GetVkDevice(), m_commandPool, nullptr);
	m_state = SystemState::Uninitialized;
}

ERROR_CODE VulkanCommand::CreateCommandPool() {
	auto indices = ref_device->GetQueueFamilies();

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	poolInfo.flags			  = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

	if (vkCreateCommandPool(ref_device->GetVkDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create graphics command pool!");
		return ERROR_CODE::VULKAN_COMMAND_CREATION_FAILED;
	}
	return ERROR_CODE::OK;
}

ERROR_CODE VulkanCommand::CreateCommandBuffers(uint32_t frames) {
	m_commandBuffers.resize(frames);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType				 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool		 = m_commandPool;
	allocInfo.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = frames;

	if (vkAllocateCommandBuffers(ref_device->GetVkDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to allocate command buffers!");
		return ERROR_CODE::VULKAN_COMMAND_CREATION_FAILED;
	}
	return ERROR_CODE::OK;
}
}  // namespace PE::Graphics::Vulkan