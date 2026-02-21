#pragma once
#include <filesystem>
#include <unordered_map>

#include "AssetInfo.h"
#include "Common/Common.h"
#include "Core/EngineConfig.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RenderTypes.h"

namespace PE::Scene {
struct MaterialConfigBuilder;
}

namespace PE::Assets {
using ModelID = uint32_t;

class AssetManager {
public:
	AssetManager() = delete;

	static ERROR_CODE Initialize(Graphics::IRenderer *renderer, const Core::EngineConfig &engineConfig);
	static ERROR_CODE Shutdown();

	static Graphics::TextureID	RequestDefaultTexture(Graphics::TextureType type = Graphics::TextureType::Albedo);
	static Graphics::MeshID		RequestPrimitiveMesh(Graphics::PrimitiveType type = Graphics::PrimitiveType::Box,
													 float radius = 1.0, float width = 1.0, float height = 1.0,
													 float depth = 1.0, int sliceCount = 8, int stackCount = 8);
	static Graphics::ShaderID	RequestDefaultShader();
	static Graphics::ShaderID	RequestParticleShader();
	static Graphics::MaterialID RequestDefaultMaterial();

	static Graphics::TextureID RequestTexture(const std::string &name, const std::vector<std::filesystem::path> &paths,
											  const Graphics::TextureParameters &params);
	static Graphics::MeshID	   RequestMesh(const std::string &name);
	static Graphics::MeshID	   RequestMesh(const std::string &name, const Graphics::MeshData &meshData);
	static Graphics::ShaderID  RequestShader(const std::string &name, Graphics::ShaderType type,
											 const std::filesystem::path &vsPath, const std::filesystem::path &psPath);

	static Graphics::MaterialID RequestMaterial(const std::string	   &name	   = DefaultMaterialName.data(),
												const std::string_view &shaderName = DefaultShaderName.data());

	static Graphics::MaterialID RequestMaterial(
		const std::string																				 &name,
		const std::unordered_map<Graphics::MaterialProperty,
								 std::variant<float, int, Math ::Vector2, Math::Vector3, Math::Vector4>> &matProperties,
		std::unordered_map<Graphics::TextureType, std::pair<std::string, std::vector<std::filesystem::path>>>
						  &textureBindings,
		const std::string &shaderName = DefaultShaderName.data());
	static Graphics::MaterialID RequestMaterial(const Scene::MaterialConfigBuilder &builder);
	static ModelAssetInfo	   *RequestModel(std::string &modelName, const std::filesystem::path &path,
											 const std::string &shaderName = DefaultShaderName.data());

	static Graphics::TextureID	GetTextureHandle(const std::string &fileName);
	static Graphics::ShaderID	GetShaderHandle(const std::string &fileName);
	static Graphics::MaterialID GetMaterialHandle(const std::string &fileName);
	static Graphics::MeshID		GetMeshHandle(const std::string &fileName);
	static ModelAssetInfo	   *GetModelAssetInfo(const std::string &fileName);

	static const auto &GetTextureRegistry() { return s_texAssetRegistry; }
	static const auto &GetMeshRegistry() { return s_meshAssetRegistry; }
	static const auto &GetMaterialRegistry() { return s_matAssetRegistry; }
	static const auto &GetShaderRegistry() { return s_shaderAssetRegistry; }
	static const auto &GetModelRegistry() { return s_modelAssetRegistry; }

	static inline constexpr std::string_view	 DefaultShaderName		   = "Default_Phong_Forward";
	static inline constexpr std::string_view	 DefaultUnlitShaderName	   = "Default_Unlit";
	static inline constexpr std::string_view	 DefaultParticleShaderName = "Default_Particle";
	static inline constexpr std::string_view	 DefaultShadowShaderName   = "Default_Shadow";
	static inline constexpr Graphics::ShaderType DefaultShaderType		   = Graphics::ShaderType::Lit;
	static inline constexpr std::string_view	 DefaultMaterialName	   = "Default_Phong";
	static inline constexpr std::string_view	 DefaultQuadName		   = "Default_Quad";

	static inline Graphics::ShaderID   DefaultShaderID		   = Graphics::INVALID_HANDLE;
	static inline Graphics::ShaderID   DefaultUnlitShaderID	   = Graphics::INVALID_HANDLE;
	static inline Graphics::ShaderID   DefaultParticleShaderID = Graphics::INVALID_HANDLE;
	static inline Graphics::ShaderID   DefaultShadowShaderID   = Graphics::INVALID_HANDLE;
	static inline Graphics::MaterialID DefaultMaterialID	   = Graphics::INVALID_HANDLE;
	static inline Graphics::MaterialID DefaultQuadID		   = Graphics::INVALID_HANDLE;

	static inline constexpr std::string_view	 ErrorTextureName  = "Error_Texture";
	static inline constexpr std::string_view	 ErrorShaderName   = "Error_Shader";
	static inline constexpr std::string_view	 ErrorMaterialName = "Error_Material";
	static inline constexpr Graphics::ShaderType ErrorShaderType   = Graphics::ShaderType::Lit;
	static inline Graphics::ShaderID			 ErrorShaderID	   = Graphics::INVALID_HANDLE;
	static inline Graphics::MaterialID			 ErrorMaterialID   = Graphics::INVALID_HANDLE;

private:
	static void ReserveMemory(size_t textureCount, size_t meshCount, size_t materialCount, size_t modelCount,
							  size_t shaderCount);
	template <typename T>
	static T   *AllocateAsset(std::vector<T> &store);
	static void CreateDefaultRenderAssets();
	static void CreateDefaultTextures();
	static void CreateDefaultShaders();
	static void CreateDefaultMaterials();
	static void CreateDefaultMeshes();

	static inline Graphics::IRenderer *ref_renderer = nullptr;
	static inline SystemState		   s_state		= SystemState::Uninitialized;

	static inline std::unordered_map<std::string, TextureAssetInfo *>  s_texAssetRegistry;
	static inline std::unordered_map<std::string, MeshAssetInfo *>	   s_meshAssetRegistry;
	static inline std::unordered_map<std::string, MaterialAssetInfo *> s_matAssetRegistry;
	static inline std::unordered_map<std::string, ShaderAssetInfo *>   s_shaderAssetRegistry;
	static inline std::unordered_map<std::string, ModelAssetInfo *>	   s_modelAssetRegistry;

	static inline std::vector<ModelID>	   s_models;
	static inline std::vector<std::string> s_defaultTextureNames;

	static inline std::vector<TextureAssetInfo>	 s_textureStore;
	static inline std::vector<MeshAssetInfo>	 s_meshStore;
	static inline std::vector<MaterialAssetInfo> s_materialStore;
	static inline std::vector<ShaderAssetInfo>	 s_shaderStore;
	static inline std::vector<ModelAssetInfo>	 s_modelStore;

	static inline std::unordered_map<Graphics::TextureID, AssetInfo *>	s_texturesById;
	static inline std::unordered_map<Graphics::ShaderID, AssetInfo *>	s_shadersById;
	static inline std::unordered_map<Graphics::MaterialID, AssetInfo *> s_materialsById;
	static inline std::unordered_map<Graphics::MeshID, AssetInfo *>		s_meshesById;
	static inline std::unordered_map<ModelID, AssetInfo *>				s_modelsById;
};
}  // namespace PE::Assets