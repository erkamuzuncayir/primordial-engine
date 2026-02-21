#pragma once
#include <vulkan/vulkan.h>

#include "Common/Common.h"

namespace PE::Graphics::Vulkan {
class VulkanBuffer {
public:
	VulkanBuffer()								  = default;
	VulkanBuffer(const VulkanBuffer &)			  = delete;
	VulkanBuffer &operator=(const VulkanBuffer &) = delete;
	VulkanBuffer(VulkanBuffer &&other) noexcept;
	VulkanBuffer &operator=(VulkanBuffer &&other) noexcept;
	~VulkanBuffer() = default;
	ERROR_CODE Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
						  VkSharingMode sharingMode, VkMemoryPropertyFlags properties);
	void	   Shutdown();
	[[nodiscard]] uint32_t GetSize() const { return static_cast<uint32_t>(m_size); }

	[[nodiscard]] VkBuffer		 GetBuffer() const { return m_buffer; }
	[[nodiscard]] VkDeviceMemory GetMemory() const { return m_memory; }
	[[nodiscard]] void			*GetMappedData() const { return m_mappedData; }
	/**
	 * @brief Updates the buffer data.
	 * * If the buffer is HOST_VISIBLE (CPU accessible), this maps and copies directly.
	 * If the buffer is DEVICE_LOCAL (GPU only), this requires a staging buffer and a command buffer.
	 * * @note For DEVICE_LOCAL buffers, you MUST provide the command pool and queue.
	 * If you call this on a DEVICE_LOCAL buffer without providing them, it will fail/log error.
	 */
	void Update(const void *data, size_t size, size_t offset = 0);

	/**
	 * @brief Updates a DEVICE_LOCAL buffer using a staging buffer.
	 * This is the high-performance path for static geometry (Vertex/Index buffers).
	 */
	void UpdateStaged(VkCommandPool commandPool, VkQueue queue, const void *data, size_t size, VkDeviceSize dstOffset);

	/**
	 * @brief Maps the memory (if host visible).
	 * Useful for persistent mapping of uniform buffers.
	 */
	ERROR_CODE Map();

	/**
	 * @brief Unmaps the memory.
	 */
	void Unmap();

	/**
	 * @brief Writes to mapped memory.
	 * Assumes Map() has been called or buffer is persistently mapped.
	 */
	void WriteToMapped(const void *data, size_t size, size_t offset = 0);

private:
	ERROR_CODE CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkSharingMode sharingMode,
							VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
	void CopyBuffer(VkCommandPool commandPool, VkQueue queue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
					VkDeviceSize dstOffset);
	uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	VkDevice		 ref_vkDevice		= VK_NULL_HANDLE;
	VkPhysicalDevice ref_physicalDevice = VK_NULL_HANDLE;

	SystemState			  m_state  = SystemState::Uninitialized;
	VkBuffer			  m_buffer = VK_NULL_HANDLE;
	VkDeviceMemory		  m_memory = VK_NULL_HANDLE;
	VkDeviceSize		  m_size   = 0;
	VkBufferUsageFlags	  m_usage  = 0;
	VkSharingMode		  m_sharingMode;
	VkMemoryPropertyFlags m_properties = 0;
	void				 *m_mappedData = nullptr;
};
}  // namespace PE::Graphics::Vulkan