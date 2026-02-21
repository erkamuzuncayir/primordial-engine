#include "Graphics/Material.h"

#include <array>

#include "Common/Common.h"
#include "Graphics/IRenderer.h"

namespace PE::Graphics {
Material::Material(Material &&other) noexcept
	: ref_renderer(other.ref_renderer),
	  m_id(other.m_id),
	  m_shaderID(other.m_shaderID),
	  m_sortKeyMask(other.m_sortKeyMask),
	  m_textures(other.m_textures),
	  m_propertyBuffer(std::move(other.m_propertyBuffer)),
	  m_layout(other.m_layout) {
	other.ref_renderer	= nullptr;
	other.m_id			= INVALID_HANDLE;
	other.m_shaderID	= INVALID_HANDLE;
	other.m_sortKeyMask = 0;
	other.m_textures.fill(INVALID_HANDLE);
}

Material &Material::operator=(Material &&other) noexcept {
	if (this != &other) {
		Shutdown();

		ref_renderer	 = other.ref_renderer;
		m_id			 = other.m_id;
		m_shaderID		 = other.m_shaderID;
		m_sortKeyMask	 = other.m_sortKeyMask;
		m_textures		 = other.m_textures;
		m_propertyBuffer = std::move(other.m_propertyBuffer);
		m_layout		 = other.m_layout;

		other.ref_renderer	= nullptr;
		other.m_id			= INVALID_HANDLE;
		other.m_shaderID	= INVALID_HANDLE;
		other.m_sortKeyMask = 0;
		other.m_textures.fill(INVALID_HANDLE);
		other.m_layout.fill({});
	}
	return *this;
}

ERROR_CODE Material::Initialize(
	IRenderer *renderer, const uint32_t id, const ShaderID shaderID, const uint32_t bufferSize,
	const std::array<MaterialPropertyLayout, static_cast<size_t>(MaterialProperty::Count)> &layout) {
	ref_renderer = renderer;
	m_id		 = id;
	m_shaderID	 = shaderID;

	m_textures.fill(INVALID_HANDLE);
	m_layout.fill({0, 0});
	m_samplers.fill(SamplerType::LinearRepeat);

	m_propertyBuffer.resize(bufferSize);
	std::memset(m_propertyBuffer.data(), 0, bufferSize);

	m_layout = layout;

	uint64_t layer = 1;
	m_sortKeyMask = RenderKey::Create(static_cast<uint8_t>(static_cast<RenderPass>(0)), static_cast<uint16_t>(shaderID),
									  static_cast<uint16_t>(id), 0);

	return ERROR_CODE::OK;
}

void Material::Shutdown() {
	m_id		  = INVALID_HANDLE;
	m_shaderID	  = INVALID_HANDLE;
	m_sortKeyMask = 0;
	m_propertyBuffer.clear();
	m_textures.fill(INVALID_HANDLE);
	m_layout.fill({0, 0});
}

void Material::SetTexture(const TextureType type, const TextureID textureID) {
	if (static_cast<size_t>(type) < m_textures.size()) {
		m_textures[static_cast<size_t>(type)] = textureID;

		ref_renderer->UpdateMaterialTexture(m_id, type, textureID);
	}
}

void Material::SetSampler(const TextureType type, const SamplerType samplerType) {
	m_samplers[static_cast<uint8_t>(type)] = samplerType;
}

uint64_t Material::GetSortKeyMask() const { return m_sortKeyMask; }

ShaderID Material::GetShaderID() const { return m_shaderID; }

const std::array<TextureID, static_cast<size_t>(TextureType::Count)> &Material::GetTextures() const {
	return m_textures;
}

const std::vector<uint8_t> &Material::GetPropertyData() const { return m_propertyBuffer; }

void Material::SetPropertyInternal(MaterialProperty prop, const void *data, const size_t size) {
	size_t index = static_cast<size_t>(prop);

	if (index >= m_layout.size()) {
		return;
	}

	const auto &propLayout = m_layout[index];

	if (propLayout.size == 0) {
		return;
	}

	if (propLayout.size != size) {
		PE_LOG_ERROR("Material Property Size Mismatch");
		return;
	}

	std::memcpy(m_propertyBuffer.data() + propLayout.offset, data, size);
}
}  // namespace PE::Graphics