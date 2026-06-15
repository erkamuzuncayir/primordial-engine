#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "GUID.h"
#include "Graphics/RenderTypes.h"
#include "Math/Math.h"

namespace PE::Assets {

enum class AssetType { Unknown = 0, Texture, Mesh, Material, Shader, Model, Scene, Count };

using AssetHandle = std::variant<std::monostate, Graphics::MaterialID, Graphics::TextureID, Graphics::ShaderID, Graphics::MeshID>;

struct AssetInfo {
	explicit                           AssetInfo(const AssetType type) : type(type) {}
	virtual                            ~AssetInfo()						  = default;
	GUID                               guid = INVALID_GUID;
	AssetType                          type = AssetType::Unknown;
	std::string                        name;
	std::vector<std::filesystem::path> paths;
	AssetHandle                        ref_handle = std::monostate();
	[[nodiscard]] bool                 IsLoaded() const { return !std::holds_alternative<std::monostate>(ref_handle); }
};

struct TextureAssetInfo : AssetInfo {
	TextureAssetInfo() : AssetInfo(AssetType::Texture) {}
	Graphics::TextureParameters params{};
};

struct MeshAssetInfo : AssetInfo {
	MeshAssetInfo() : AssetInfo(AssetType::Mesh) {}
	uint32_t vertexCount = 0;
	uint32_t indexCount	 = 0;
};

struct ShaderAssetInfo : AssetInfo {
	ShaderAssetInfo() : AssetInfo(AssetType::Shader) {}
	Graphics::ShaderType shaderType = Graphics::ShaderType::Lit;
};

struct MaterialAssetInfo : AssetInfo {
	MaterialAssetInfo() : AssetInfo(AssetType::Material) {}
	GUID shaderGuid = INVALID_GUID;

	std::unordered_map<Graphics::TextureType, std::pair<std::string, std::vector<std::filesystem::path>>>
		textureBindings;
	using PropValue = std::variant<float, int, Math::Vec2, Math::Vec3, Math::Vec4>;
	std::unordered_map<Graphics::MaterialProperty, PropValue> properties;
};

struct ModelAssetInfo : AssetInfo {
	ModelAssetInfo() : AssetInfo(AssetType::Model) {}

	struct SubMeshEntry {
		GUID meshGuid	  = INVALID_GUID;
		GUID materialGuid = INVALID_GUID;
	};

	std::vector<SubMeshEntry> subMeshes;
};
}  // namespace PE::Assets