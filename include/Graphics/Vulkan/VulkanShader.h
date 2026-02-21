#pragma once
#include <filesystem>
#include <vector>

#include "Common/Common.h"
#include "Graphics/RenderTypes.h"

namespace PE::Graphics::Vulkan {
class VulkanShader {
public:
	VulkanShader()								  = default;
	VulkanShader(const VulkanShader &)			  = delete;
	VulkanShader &operator=(const VulkanShader &) = delete;
	VulkanShader(VulkanShader &&other) noexcept;
	VulkanShader &operator=(VulkanShader &&other) noexcept;
	~VulkanShader() = default;

	ERROR_CODE Initialize(VkDevice vkDevice, ShaderType type, const std::filesystem::path &vertPath,
						  const std::filesystem::path &fragPath);

	void Shutdown();

	// TODO: Implement this
	void UpdateMaterialBuffer(ShaderType type, VkDevice vkDevice, const void *data);

	[[nodiscard]] VkShaderModule GetVertModule() const { return m_vertModule; }
	[[nodiscard]] VkShaderModule GetFragModule() const { return m_fragModule; }
	[[nodiscard]] ShaderType	 GetType() const { return m_type; }

private:
	ERROR_CODE CreateShaderModule(const std::vector<char> &code, VkShaderModule &shaderModule);

	VkDevice ref_vkDevice = VK_NULL_HANDLE;

	SystemState	   m_state = SystemState::Uninitialized;
	ShaderType	   m_type;
	VkShaderModule m_vertModule = VK_NULL_HANDLE;
	VkShaderModule m_fragModule = VK_NULL_HANDLE;
};
}  // namespace PE::Graphics::Vulkan