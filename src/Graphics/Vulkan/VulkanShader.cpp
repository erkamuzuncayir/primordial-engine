#include "Graphics/Vulkan/VulkanShader.h"

#include "Utilities/IOUtilities.h"
#include "Utilities/Logger.h"

namespace PE::Graphics::Vulkan {
VulkanShader::VulkanShader(VulkanShader &&other) noexcept
	: m_state(other.m_state),
	  m_type(other.m_type),
	  ref_vkDevice(other.ref_vkDevice),
	  m_vertModule(other.m_vertModule),
	  m_fragModule(other.m_fragModule) {
	other.m_state	   = SystemState::Uninitialized;
	other.ref_vkDevice = VK_NULL_HANDLE;
	other.m_vertModule = VK_NULL_HANDLE;
	other.m_fragModule = VK_NULL_HANDLE;
}

VulkanShader &VulkanShader::operator=(VulkanShader &&other) noexcept {
	if (this != &other) {
		Shutdown();

		m_state		 = other.m_state;
		m_type		 = other.m_type;
		ref_vkDevice = other.ref_vkDevice;
		m_vertModule = other.m_vertModule;
		m_fragModule = other.m_fragModule;

		other.m_state	   = SystemState::Uninitialized;
		other.ref_vkDevice = VK_NULL_HANDLE;
		other.m_vertModule = VK_NULL_HANDLE;
		other.m_fragModule = VK_NULL_HANDLE;
	}
	return *this;
}

ERROR_CODE VulkanShader::Initialize(const VkDevice vkDevice, ShaderType type, const std::filesystem::path &vertPath,
									const std::filesystem::path &fragPath) {
	PE_CHECK_STATE_INIT(m_state, "This vulkan shader is already initialized.");
	m_state = SystemState::Initializing;

	ref_vkDevice = vkDevice;
	m_type		 = type;

	std::vector<char> vertCode, fragCode;
	ERROR_CODE		  result;
	PE_CHECK(result, Utilities::IOUtilities::ReadBinaryFile(vertPath, vertCode));
	PE_CHECK(result, Utilities::IOUtilities::ReadBinaryFile(fragPath, fragCode));
	PE_ENSURE_INIT_SILENT(result, CreateShaderModule(vertCode, m_vertModule));
	PE_ENSURE_INIT_SILENT(result, CreateShaderModule(fragCode, m_fragModule));

	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

void VulkanShader::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return;
	m_state = SystemState::ShuttingDown;

	if (ref_vkDevice) {
		if (m_vertModule) vkDestroyShaderModule(ref_vkDevice, m_vertModule, nullptr);
		if (m_fragModule) vkDestroyShaderModule(ref_vkDevice, m_fragModule, nullptr);
	}
	m_vertModule = VK_NULL_HANDLE;
	m_fragModule = VK_NULL_HANDLE;
	m_state		 = SystemState::Uninitialized;
}

ERROR_CODE VulkanShader::CreateShaderModule(const std::vector<char> &code, VkShaderModule &shaderModule) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode	= reinterpret_cast<const uint32_t *>(code.data());
	if (vkCreateShaderModule(ref_vkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan shader module can't create!");
		return ERROR_CODE::VULKAN_SHADER_PIPELINE_FAILED;
	}
	return ERROR_CODE::OK;
}
}  // namespace PE::Graphics::Vulkan