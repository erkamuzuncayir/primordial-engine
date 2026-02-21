#include "Graphics/Vulkan/VulkanBuffer.h"

#include <cstring>

#include "Utilities/Logger.h"

namespace PE::Graphics::Vulkan {
VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept
	: ref_vkDevice(other.ref_vkDevice),
	  ref_physicalDevice(other.ref_physicalDevice),
	  m_buffer(other.m_buffer),
	  m_memory(other.m_memory),
	  m_size(other.m_size),
	  m_usage(other.m_usage),
	  m_sharingMode(other.m_sharingMode),
	  m_mappedData(other.m_mappedData) {
	other.m_buffer	   = VK_NULL_HANDLE;
	other.m_memory	   = VK_NULL_HANDLE;
	other.m_mappedData = nullptr;
	other.m_size	   = 0;
}

VulkanBuffer &VulkanBuffer::operator=(VulkanBuffer &&other) noexcept {
	if (this != &other) {
		Shutdown();

		ref_vkDevice	   = other.ref_vkDevice;
		ref_physicalDevice = other.ref_physicalDevice;
		m_buffer		   = other.m_buffer;
		m_memory		   = other.m_memory;
		m_size			   = other.m_size;
		m_usage			   = other.m_usage;
		m_sharingMode	   = other.m_sharingMode;
		m_mappedData	   = other.m_mappedData;

		other.m_buffer	   = VK_NULL_HANDLE;
		other.m_memory	   = VK_NULL_HANDLE;
		other.m_mappedData = nullptr;
		other.m_size	   = 0;
	}
	return *this;
}

ERROR_CODE VulkanBuffer::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size,
									VkBufferUsageFlags usage, VkSharingMode sharingMode,
									VkMemoryPropertyFlags properties) {
	PE_CHECK_STATE_INIT(m_state, "This vulkan buffer is already initialized.");
	m_state = SystemState::Initializing;

	ref_vkDevice	   = device;
	ref_physicalDevice = physicalDevice;
	m_size			   = size;
	m_usage			   = usage;
	m_sharingMode	   = sharingMode;
	m_properties	   = properties;

	ERROR_CODE result;
	PE_ENSURE_INIT_SILENT(result, CreateBuffer(size, usage, sharingMode, properties, m_buffer, m_memory));

	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

void VulkanBuffer::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return;
	m_state = SystemState::ShuttingDown;
	if (ref_vkDevice) {
		if (m_buffer != VK_NULL_HANDLE) vkDestroyBuffer(ref_vkDevice, m_buffer, nullptr);
		if (m_memory != VK_NULL_HANDLE) vkFreeMemory(ref_vkDevice, m_memory, nullptr);
	}
	m_buffer	 = VK_NULL_HANDLE;
	m_memory	 = VK_NULL_HANDLE;
	m_mappedData = nullptr;
	m_state		 = SystemState::Uninitialized;
}

void VulkanBuffer::Update(const void *data, size_t size, size_t offset) {
	if (m_mappedData) {
		WriteToMapped(data, size, offset);
	} else {
		PE_LOG_ERROR("VulkanBuffer::Update called on unmapped buffer. Use UpdateStaged for Device Local memory.");
	}
}

void VulkanBuffer::UpdateStaged(VkCommandPool commandPool, VkQueue queue, const void *data, VkDeviceSize size,
								VkDeviceSize dstOffset) {
	VkBuffer	   stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE,
				 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
				 stagingBufferMemory);

	void *mappedData;
	vkMapMemory(ref_vkDevice, stagingBufferMemory, 0, size, 0, &mappedData);
	memcpy(mappedData, data, size);
	vkUnmapMemory(ref_vkDevice, stagingBufferMemory);

	CopyBuffer(commandPool, queue, stagingBuffer, m_buffer, size, dstOffset);

	vkDestroyBuffer(ref_vkDevice, stagingBuffer, nullptr);
	vkFreeMemory(ref_vkDevice, stagingBufferMemory, nullptr);
}

ERROR_CODE VulkanBuffer::Map() {
	if (m_mappedData) return ERROR_CODE::VULKAN_BUFFER_CREATION_FAILED;

	if (vkMapMemory(ref_vkDevice, m_memory, 0, m_size, 0, &m_mappedData) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to map memory!");
		return ERROR_CODE::VULKAN_BUFFER_CREATION_FAILED;
	}

	return ERROR_CODE::OK;
}

void VulkanBuffer::Unmap() {
	if (!m_mappedData) return;
	vkUnmapMemory(ref_vkDevice, m_memory);
	m_mappedData = nullptr;
}

void VulkanBuffer::WriteToMapped(const void *data, size_t size, size_t offset) {
	if (m_mappedData) {
		size_t copySize = (size > m_size) ? m_size : size;
		memcpy(static_cast<char *>(m_mappedData) + offset, data, copySize);
	}
}

ERROR_CODE VulkanBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode,
									  VkMemoryPropertyFlags properties, VkBuffer &buffer,
									  VkDeviceMemory &bufferMemory) {
	const VkBufferCreateInfo bufferInfo{
		.sType				   = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext				   = nullptr,
		.flags				   = 0,
		.size				   = size,
		.usage				   = usage,
		.sharingMode		   = sharingMode,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices   = nullptr,
	};

	if (vkCreateBuffer(ref_vkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create buffer!");
		return ERROR_CODE::VULKAN_BUFFER_CREATION_FAILED;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(ref_vkDevice, buffer, &memRequirements);

	const VkMemoryAllocateInfo allocInfo{
		.sType			 = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext			 = nullptr,
		.allocationSize	 = memRequirements.size,
		.memoryTypeIndex = FindMemoryType(ref_physicalDevice, memRequirements.memoryTypeBits, properties)};

	if (vkAllocateMemory(ref_vkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to allocate buffer memory!");
		return ERROR_CODE::VULKAN_BUFFER_CREATION_FAILED;
	}

	if (vkBindBufferMemory(ref_vkDevice, buffer, bufferMemory, 0) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to bind buffer memory!");
		return ERROR_CODE::VULKAN_BUFFER_CREATION_FAILED;
	}

	return ERROR_CODE::OK;
}

void VulkanBuffer::CopyBuffer(VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer,
							  VkDeviceSize size, VkDeviceSize dstOffset) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType				 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool		 = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	if (vkAllocateCommandBuffers(ref_vkDevice, &allocInfo, &commandBuffer) != VK_FALSE)
		PE_LOG_FATAL("Vulkan can't allocate buffer!");

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size		 = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType			  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &commandBuffer;

	if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_FALSE) PE_LOG_FATAL("Vulkan can't copy buffer!");

	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(ref_vkDevice, commandPool, 1, &commandBuffer);
}

uint32_t VulkanBuffer::FindMemoryType(const VkPhysicalDevice physicalDevice, const uint32_t typeFilter,
									  const VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	PE_LOG_FATAL("Vulkan failed to find suitable memory type!");
	return 0;
}
}  // namespace PE::Graphics::Vulkan