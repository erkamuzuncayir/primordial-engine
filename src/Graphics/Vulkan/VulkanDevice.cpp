#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

#include <map>
#include <set>

#include "Graphics/Vulkan/VulkanDevice.h"
#include "Utilities/Logger.h"

namespace PE::Graphics::Vulkan {
VkResult CreateDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
									  const VkAllocationCallbacks *pAllocator,
									  VkDebugUtilsMessengerEXT	  *pDebugMessenger) {
	if (const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		func != nullptr)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);

	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(const VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger,
								   const VkAllocationCallbacks *pAllocator) {
	const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
	if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

ERROR_CODE VulkanDevice::Initialize(GLFWwindow *windowHandle) {
	PE_CHECK_STATE_INIT(m_state, "Vulkan device is already initialized.");
	m_state	   = SystemState::Initializing;
	ref_window = windowHandle;
	ERROR_CODE result;
	PE_CHECK(result, CreateInstance());
	if (m_enableValidationLayers) PE_CHECK(result, SetupDebugMessenger());

	PE_CHECK(result, CreateSurface());
	PE_CHECK(result, PickPhysicalDevice());
	PE_CHECK(result, CreateLogicalDevice());

	m_state = SystemState::Running;
	PE_LOG_INFO("Vulkan Device Initialized.");
	return result;
}

void VulkanDevice::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return;

	m_state = SystemState::ShuttingDown;
	vkDestroyDevice(m_vkDevice, nullptr);
	if (m_enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	}
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	m_state = SystemState::Uninitialized;
	PE_LOG_INFO("Vulkan Device Shutdown.");
}

uint32_t VulkanDevice::FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	PE_LOG_ERROR("failed to find suitable memory type!");
	return 0;
}

SwapChainSupportDetails VulkanDevice::QuerySwapChainSupport(const VkPhysicalDevice physicalDevice) {
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &details.capabilities);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, nullptr);
	if (formatCount) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, m_surface, &formatCount, details.formats.data());
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, nullptr);
	if (presentModeCount) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount,
												  details.presentModes.data());
	}
	return details;
}

ERROR_CODE VulkanDevice::CreateInstance() {
#if !NDEBUG
	LogEverySupportedExtensions();
#endif

	if (m_enableValidationLayers && !CheckValidationLayerSupport()) {
		PE_LOG_ERROR("validation layers requested, but not available!");
		return ERROR_CODE::VULKAN_DEVICE_CREATION_FAILED;
	}

	VkApplicationInfo appInfo{};
	appInfo.sType			   = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName   = "Primordial Engine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName		   = "Primordial Engine";
	appInfo.engineVersion	   = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion		   = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType			= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	const auto extensions			   = GetRequiredExtensions();
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (m_enableValidationLayers) {
		createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT *>(&debugCreateInfo);
	} else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext			 = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create instance!");
		return ERROR_CODE::VULKAN_DEVICE_CREATION_FAILED;
	}
	return ERROR_CODE::OK;
}

ERROR_CODE VulkanDevice::SetupDebugMessenger() {
	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to set up debug messenger!");
		return ERROR_CODE::VULKAN_DEVICE_CREATION_FAILED;
	}
	return ERROR_CODE::OK;
}

ERROR_CODE VulkanDevice::CreateSurface() {
	if (glfwCreateWindowSurface(m_instance, (GLFWwindow *)ref_window, nullptr, &m_surface) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create window surface!");
		return ERROR_CODE::VULKAN_DEVICE_CREATION_FAILED;
	}

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanDevice::PickPhysicalDevice() {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		PE_LOG_FATAL("Vulkan failed to find GPUs with Vulkan support!");
		return ERROR_CODE::VULKAN_DEVICE_CREATION_FAILED;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	if (deviceCount < 2) {
		if (IsDeviceSuitable(devices[0])) {
			m_vkPhysicalDevice = devices[0];
		}
	} else {
		std::multimap<uint32_t, VkPhysicalDevice> candidates;
		for (const auto &device : devices) {
			if (IsDeviceSuitable(device)) {
				uint32_t score = RateDeviceSuitability(device);
				candidates.insert(std::make_pair(score, device));
			}
		}

		if (!candidates.empty() && candidates.rbegin()->first > 0) {
			m_vkPhysicalDevice = candidates.rbegin()->second;
		}
	}

	if (m_vkPhysicalDevice == VK_NULL_HANDLE) {
		PE_LOG_FATAL("Vulkan failed to find a suitable GPU!");
		return ERROR_CODE::VULKAN_DEVICE_CREATION_FAILED;
	}

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanDevice::CreateLogicalDevice() {
	m_indices = FindQueueFamilies(m_vkPhysicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {m_indices.graphicsFamily.value(), m_indices.presentFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType			 = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount		 = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceSynchronization2Features sync2Features{};
	sync2Features.sType			   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	sync2Features.synchronization2 = VK_TRUE;

	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
	dynamicRenderingFeatures.sType			  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
	dynamicRenderingFeatures.pNext			  = &sync2Features;

	VkPhysicalDeviceFeatures2 deviceFeatures2{};
	deviceFeatures2.sType					   = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures2.pNext					   = &dynamicRenderingFeatures;
	deviceFeatures2.features.samplerAnisotropy = VK_TRUE;
	deviceFeatures2.features.fillModeNonSolid  = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType				   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext				   = &deviceFeatures2;
	createInfo.queueCreateInfoCount	   = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos	   = queueCreateInfos.data();
	createInfo.pEnabledFeatures		   = nullptr;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(m_deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();

	if (m_enableValidationLayers) {
		createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create logical device!");
		return ERROR_CODE::VULKAN_DEVICE_CREATION_FAILED;
	}

	vkGetDeviceQueue(m_vkDevice, m_indices.graphicsFamily.value(), 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_vkDevice, m_indices.presentFamily.value(), 0, &m_presentQueue);

	return ERROR_CODE::OK;
}

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice physicalDevice) {
	m_indices				 = FindQueueFamilies(physicalDevice);
	bool extensionsSupported = CheckDeviceExtensionSupport(physicalDevice);
	bool swapChainAdequate	 = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(physicalDevice);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}
	return m_indices.IsComplete() && extensionsSupported && swapChainAdequate;
}

uint32_t VulkanDevice::RateDeviceSuitability(VkPhysicalDevice physicalDevice) {
	uint32_t				   score = 0;
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	score += deviceProperties.limits.maxImageDimension2D;

	return score;
}

bool VulkanDevice::CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());
	for (const auto &extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}
	return requiredExtensions.empty();
}

VulkanDevice::QueueFamilyIndices VulkanDevice::FindQueueFamilies(VkPhysicalDevice physicalDevice) {
	QueueFamilyIndices indices;
	uint32_t		   queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, m_surface, &presentSupport);
		if (presentSupport) {
			indices.presentFamily = i;
		}
		if (indices.IsComplete()) break;
		i++;
	}
	return indices;
}

bool VulkanDevice::CheckValidationLayerSupport() const {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char *layerName : m_validationLayers) {
		bool layerFound = false;
		for (const auto &layerProperties : availableLayers) {
			if (std::string_view(layerName) == layerProperties.layerName) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) return false;
	}
	return true;
}

std::vector<const char *> VulkanDevice::GetRequiredExtensions() const {
	uint32_t	 glfwExtensionCount = 0;
	const char **glfwExtensions		= glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	if (m_enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extensions;
}

void VulkanDevice::LogEverySupportedExtensions() {
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	PE_LOG_DEBUG("available extensions:\n");
	for (const auto &[extensionName, specVersion] : extensions) {
		PE_LOG_DEBUG(std::string(extensionName) + "\t" + std::to_string(specVersion));
	}
}

void VulkanDevice::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
	createInfo				   = {};
	createInfo.sType		   = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDevice::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT	   messageSeverity,
														   VkDebugUtilsMessageTypeFlagsEXT			   messageType,
														   const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
														   void										  *pUserData) {
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		PE_LOG_ERROR(std::string("[VULKAN VALIDATION] ") + pCallbackData->pMessage);
	} else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		PE_LOG_WARN(std::string("[VULKAN VALIDATION] ") + pCallbackData->pMessage);
	}
	return VK_FALSE;
}
}  // namespace PE::Graphics::Vulkan