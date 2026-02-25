#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "Graphics/RenderTypes.h"
#include "Math/Math.h"

namespace PE::Assets {
using AssetGUID					 = uint64_t;
constexpr AssetGUID INVALID_GUID = UINT64_MAX;

enum class AssetType { Unknown = 0, Texture, Mesh, Material, Shader, Model, Scene, Count };

struct AssetInfo {
	virtual ~AssetInfo()						  = default;
	AssetGUID						   guid		  = INVALID_GUID;
	AssetGUID						   parentGuid = INVALID_GUID;
	AssetType						   type		  = AssetType::Unknown;
	std::string						   name;
	std::vector<std::filesystem::path> sourcePaths;
	uint32_t						   ref_handle = Graphics::INVALID_HANDLE;
	[[nodiscard]] bool				   IsLoaded() const { return ref_handle != Graphics::INVALID_HANDLE; }
};

struct TextureAssetInfo : AssetInfo {
	TextureAssetInfo() { type = AssetType::Texture; }
	Graphics::TextureParameters params{};
};

struct MeshAssetInfo : AssetInfo {
	MeshAssetInfo() { type = AssetType::Mesh; }
	uint32_t vertexCount = 0;
	uint32_t indexCount	 = 0;
};

struct ShaderAssetInfo : AssetInfo {
	ShaderAssetInfo() { type = AssetType::Shader; }
	Graphics::ShaderType  shaderType = Graphics::ShaderType::Lit;
	std::filesystem::path vsPath;
	std::filesystem::path psPath;
};

struct MaterialAssetInfo : AssetInfo {
	MaterialAssetInfo() { type = AssetType::Material; }
	std::string shaderAssetName;

	std::unordered_map<Graphics::TextureType, std::pair<std::string, std::vector<std::filesystem::path>>>
		textureBindings;
	using PropValue = std::variant<float, int, Math::Vec2, Math::Vec3, Math::Vec4>;
	std::unordered_map<Graphics::MaterialProperty, PropValue> properties;
};

struct ModelAssetInfo : AssetInfo {
	ModelAssetInfo() { type = AssetType::Model; }

	struct SubMeshEntry {
		std::string meshAssetName;
		std::string materialAssetName;
	};

	std::vector<SubMeshEntry> subMeshes;
};
}  // namespace PE::Assets