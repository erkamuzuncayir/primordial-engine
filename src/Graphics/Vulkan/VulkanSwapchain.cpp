#ifdef _WIN32
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <algorithm>

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Utilities/Logger.h"

namespace PE::Graphics::Vulkan {
ERROR_CODE VulkanSwapchain::Initialize(GLFWwindow *windowHandle, VulkanDevice *vulkanDevice, float width,
									   float height) {
	PE_CHECK_STATE_INIT(m_state, "Vulkan swapchain is already initialized.");
	m_state = SystemState::Initializing;

	ref_window		 = windowHandle;
	ref_vulkanDevice = vulkanDevice;

	ERROR_CODE result;
	PE_ENSURE_INIT_SILENT(result, CreateSwapchain(width, height));
	PE_ENSURE_INIT_SILENT(result, CreateImageViews());

	m_state = SystemState::Running;
	return result;
}

void VulkanSwapchain::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return;
	m_state = SystemState::ShuttingDown;

	for (const auto iv : m_swapChainImageViews) vkDestroyImageView(ref_vulkanDevice->GetVkDevice(), iv, nullptr);
	if (m_vkSwapchain) vkDestroySwapchainKHR(ref_vulkanDevice->GetVkDevice(), m_vkSwapchain, nullptr);

	m_swapChainImageViews.clear();
	m_swapChainImages.clear();
	m_vkSwapchain = VK_NULL_HANDLE;
	m_state		  = SystemState::Uninitialized;
}

VkResult VulkanSwapchain::AcquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t &outImageIndex) {
	return vkAcquireNextImageKHR(ref_vulkanDevice->GetVkDevice(), m_vkSwapchain, UINT64_MAX, imageAvailableSemaphore,
								 VK_NULL_HANDLE, &outImageIndex);
}

VkResult VulkanSwapchain::Present(VkSemaphore renderFinishedSemaphore, uint32_t imageIndex) {
	VkSwapchainKHR	 swapChains[] = {m_vkSwapchain};
	VkPresentInfoKHR presentInfo  = {
		 .sType				 = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		 .waitSemaphoreCount = 1,
		 .pWaitSemaphores	 = &renderFinishedSemaphore,
		 .swapchainCount	 = 1,
		 .pSwapchains		 = swapChains,
		 .pImageIndices		 = &imageIndex,
		 .pResults			 = nullptr,
	 };

	return vkQueuePresentKHR(ref_vulkanDevice->GetPresentQueue(), &presentInfo);
}

void VulkanSwapchain::Recreate(int width, int height) {
	for (const auto iv : m_swapChainImageViews) vkDestroyImageView(ref_vulkanDevice->GetVkDevice(), iv, nullptr);

	m_swapChainImageViews.clear();
	m_swapChainImages.clear();

	VkSwapchainKHR oldSwapchain = m_vkSwapchain;
	ERROR_CODE	   result		= CreateSwapchain(width, height, oldSwapchain);
	if (result < ERROR_CODE::WARN_START) PE_LOG_FATAL("Vulkan can't recreate swapchain!");

	if (oldSwapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(ref_vulkanDevice->GetVkDevice(), oldSwapchain, nullptr);
	}

	CreateImageViews();
}

ERROR_CODE VulkanSwapchain::CreateSwapchain(const float width, const float height, VkSwapchainKHR oldSwapchain) {
	const SwapChainSupportDetails supportDetails =
		ref_vulkanDevice->QuerySwapChainSupport(ref_vulkanDevice->GetVkPhysicalDevice());
	const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(supportDetails.formats);
	const VkPresentModeKHR	 presentMode   = ChooseSwapPresentMode(supportDetails.presentModes);
	const VkExtent2D		 extent		   = ChooseSwapExtent(supportDetails.capabilities, width, height);

	uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
	if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount)
		imageCount = supportDetails.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType			= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface			= ref_vulkanDevice->GetSurface();
	createInfo.minImageCount	= imageCount;
	createInfo.imageFormat		= surfaceFormat.format;
	createInfo.imageColorSpace	= surfaceFormat.colorSpace;
	createInfo.imageExtent		= extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.oldSwapchain		= oldSwapchain;

	const auto	   indices				= ref_vulkanDevice->GetQueueFamilies();
	const uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode		 = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices	 = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode		 = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices	 = nullptr;
	}

	createInfo.preTransform	  = supportDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode	  = presentMode;
	createInfo.clipped		  = VK_TRUE;

	if (vkCreateSwapchainKHR(ref_vulkanDevice->GetVkDevice(), &createInfo, nullptr, &m_vkSwapchain) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create swap chain!");
		return ERROR_CODE::VULKAN_SWAPCHAIN_CREATION_FAILED;
	}

	vkGetSwapchainImagesKHR(ref_vulkanDevice->GetVkDevice(), m_vkSwapchain, &imageCount, nullptr);
	m_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(ref_vulkanDevice->GetVkDevice(), m_vkSwapchain, &imageCount, m_swapChainImages.data());

	m_swapChainImageFormat = surfaceFormat.format;
	m_swapChainExtent	   = extent;

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanSwapchain::CreateImageViews() {
	m_swapChainImageViews.resize(m_swapChainImages.size());
	for (size_t i = 0; i < m_swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType						   = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image						   = m_swapChainImages[i];
		createInfo.viewType						   = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format						   = m_swapChainImageFormat;
		createInfo.components.r					   = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g					   = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b					   = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a					   = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask	   = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel   = 0;
		createInfo.subresourceRange.levelCount	   = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount	   = 1;

		if (vkCreateImageView(ref_vulkanDevice->GetVkDevice(), &createInfo, nullptr, &m_swapChainImageViews[i]) !=
			VK_SUCCESS) {
			PE_LOG_FATAL("Vulkan failed to create image views!");
			return ERROR_CODE::VULKAN_SWAPCHAIN_CREATION_FAILED;
		}
	}

	return ERROR_CODE::OK;
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
	for (const auto &f : formats)
		if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) return f;
	return formats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &modes) {
	for (const auto &m : modes)
		if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, const int width,
											 const int height) {
	VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
	actualExtent.width =
		std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height =
		std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return actualExtent;
}
}  // namespace PE::Graphics::Vulkan