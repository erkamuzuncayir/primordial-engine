#pragma once
#include <optional>
#include <vector>

#include "Common/Common.h"
#include "GLFW/glfw3.h"
#include "Graphics/Vulkan/VulkanTypes.h"

namespace PE::Graphics::Vulkan {
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR		capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR>	presentModes;
};

class VulkanDevice {
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
		[[nodiscard]] bool		IsComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

public:
	VulkanDevice()								  = default;
	VulkanDevice(const VulkanDevice &)			  = delete;
	VulkanDevice &operator=(const VulkanDevice &) = delete;
	VulkanDevice(VulkanDevice &&)				  = delete;
	VulkanDevice &operator=(VulkanDevice &&)	  = delete;
	~VulkanDevice()								  = default;
	ERROR_CODE Initialize(GLFWwindow *windowHandle);
	void	   Shutdown();

	[[nodiscard]] VkInstance				GetInstance() const { return m_instance; }
	[[nodiscard]] VkPhysicalDevice			GetVkPhysicalDevice() const { return m_vkPhysicalDevice; }
	[[nodiscard]] VkDevice					GetVkDevice() const { return m_vkDevice; }
	[[nodiscard]] VkSurfaceKHR				GetSurface() const { return m_surface; }
	[[nodiscard]] VkQueue					GetGraphicsQueue() const { return m_graphicsQueue; }
	[[nodiscard]] VkQueue					GetPresentQueue() const { return m_presentQueue; }
	[[nodiscard]] const QueueFamilyIndices &GetQueueFamilies() const { return m_indices; }

	uint32_t				FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice);

private:
	ERROR_CODE CreateInstance();
	ERROR_CODE SetupDebugMessenger();
	ERROR_CODE CreateSurface();
	ERROR_CODE PickPhysicalDevice();
	ERROR_CODE CreateLogicalDevice();

	bool			   IsDeviceSuitable(VkPhysicalDevice physicalDevice);
	uint32_t		   RateDeviceSuitability(VkPhysicalDevice physicalDevice);
	bool			   CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice);

	[[nodiscard]] bool						CheckValidationLayerSupport() const;
	[[nodiscard]] std::vector<const char *> GetRequiredExtensions() const;
	void									LogEverySupportedExtensions();
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT		messageSeverity,
														VkDebugUtilsMessageTypeFlagsEXT				messageType,
														const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
														void									   *pUserData);

	GLFWwindow *ref_window = nullptr;

	SystemState				 m_state			= SystemState::Uninitialized;
	VkInstance				 m_instance			= VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_debugMessenger	= VK_NULL_HANDLE;
	VkSurfaceKHR			 m_surface			= VK_NULL_HANDLE;
	VkPhysicalDevice		 m_vkPhysicalDevice = VK_NULL_HANDLE;
	VkDevice				 m_vkDevice			= VK_NULL_HANDLE;

	VkQueue			   m_graphicsQueue = VK_NULL_HANDLE;
	VkQueue			   m_presentQueue  = VK_NULL_HANDLE;
	QueueFamilyIndices m_indices;

#ifdef NDEBUG
	const bool m_enableValidationLayers = false;
#else
	const bool m_enableValidationLayers = true;
#endif

	const std::vector<const char *> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};
	const std::vector<const char *> m_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};
}  // namespace PE::Graphics::Vulkan