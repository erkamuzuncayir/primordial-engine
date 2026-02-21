#pragma once

#include <array>
#include <vector>

#include "Common/Common.h"
#include "RenderTypes.h"

namespace PE::Graphics {
class IRenderer;

class Material {
public:
	Material()							  = default;
	Material(const Material &)			  = delete;
	Material &operator=(const Material &) = delete;
	Material(Material &&other) noexcept;
	Material &operator=(Material &&other) noexcept;
	~Material() = default;

	ERROR_CODE Initialize(IRenderer *renderer, uint32_t id, ShaderID shaderID, uint32_t bufferSize,
						  const std::array<MaterialPropertyLayout, 19> &layout);
	void	   Shutdown();

	void SetTexture(TextureType type, TextureID textureID);
	template <typename T>
	void SetProperty(MaterialProperty prop, const T &value);
	void SetSampler(TextureType type, SamplerType samplerType);

	[[nodiscard]] uint64_t																GetSortKeyMask() const;
	[[nodiscard]] ShaderID																GetShaderID() const;
	[[nodiscard]] const std::array<TextureID, static_cast<size_t>(TextureType::Count)> &GetTextures() const;
	[[nodiscard]] const std::vector<uint8_t>										   &GetPropertyData() const;
	[[nodiscard]] SamplerType GetSampler(TextureType type) const { return m_samplers[static_cast<size_t>(type)]; }

private:
	void SetPropertyInternal(MaterialProperty prop, const void *data, size_t size);

	IRenderer *ref_renderer = nullptr;

	uint32_t m_id		   = INVALID_HANDLE;
	ShaderID m_shaderID	   = INVALID_HANDLE;
	uint64_t m_sortKeyMask = 0;

	std::array<TextureID, static_cast<size_t>(TextureType::Count)>					 m_textures{};
	std::array<SamplerType, static_cast<size_t>(TextureType::Count)>				 m_samplers{};
	std::vector<uint8_t>															 m_propertyBuffer;
	std::array<MaterialPropertyLayout, static_cast<size_t>(MaterialProperty::Count)> m_layout;
};

template <typename T>
void Material::SetProperty(const MaterialProperty prop, const T &value) {
	SetPropertyInternal(prop, &value, sizeof(T));
}
}  // namespace PE::Graphics