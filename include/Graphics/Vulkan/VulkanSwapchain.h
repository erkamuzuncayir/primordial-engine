#pragma once
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>

#include "Common/Common.h"

namespace PE::Graphics::Vulkan {
class VulkanDevice;

class VulkanSwapchain {
public:
	VulkanSwapchain()  = default;
	~VulkanSwapchain() = default;

	ERROR_CODE Initialize(GLFWwindow *windowHandle, VulkanDevice *vulkanDevice, float width, float height);
	void	   Shutdown();

	VkResult AcquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t &outImageIndex);
	VkResult Present(VkSemaphore renderFinishedSemaphore, uint32_t imageIndex);

	[[nodiscard]] VkSwapchainKHR			GetHandle() const { return m_vkSwapchain; }
	[[nodiscard]] VkFormat					GetImageFormat() const { return m_swapChainImageFormat; }
	[[nodiscard]] VkExtent2D				GetExtent() const { return m_swapChainExtent; }
	[[nodiscard]] std::vector<VkImage>	   &GetImages() { return m_swapChainImages; }
	[[nodiscard]] std::vector<VkImageView> &GetImageViews() { return m_swapChainImageViews; }

	void Recreate(int width, int height);

private:
	ERROR_CODE CreateSwapchain(float width, float height, VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);
	ERROR_CODE CreateImageViews();

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
	VkPresentModeKHR   ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &modes);
	VkExtent2D		   ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, int width, int height);

private:
	GLFWwindow	 *ref_window;
	VulkanDevice *ref_vulkanDevice;

	SystemState				 m_state	   = SystemState::Uninitialized;
	VkSwapchainKHR			 m_vkSwapchain = VK_NULL_HANDLE;
	VkFormat				 m_swapChainImageFormat;
	VkExtent2D				 m_swapChainExtent;
	std::vector<VkImage>	 m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
};
}  // namespace PE::Graphics::Vulkan