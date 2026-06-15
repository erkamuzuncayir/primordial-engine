#pragma once
#include <deque>
#include <filesystem>
#include <unordered_map>

#include "AssetInfo.h"
#include "Common/Common.h"
#include "Core/EngineConfig.h"
#include "GUID.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RenderTypes.h"

namespace PE::Scene {
struct MaterialConfigBuilder;
}

namespace PE::Assets {
class AssetManager {
public:
	AssetManager() = delete;

	static ERROR_CODE Initialize(Graphics::IRenderer *renderer, const Core::EngineConfig &engineConfig);
	static ERROR_CODE Shutdown();

	static ERROR_CODE RefreshAssetDatabase();
	static ERROR_CODE ScanAssets();
	static ERROR_CODE ImportAsset();
	static Graphics::TextureID	GetDefaultTextureIDByType(Graphics::TextureType type = Graphics::TextureType::Albedo);
	static Graphics::TextureID	GetErrorTextureID();
	static Graphics::MeshID		CreatePrimitiveMesh(Graphics::PrimitiveType type = Graphics::PrimitiveType::Box,
													 float radius = 1.0, float width = 1.0, float height = 1.0,
													 float depth = 1.0, int sliceCount = 8, int stackCount = 8);
	static Graphics::ShaderID	GetDefaultShaderID();
	static Graphics::ShaderID	GetDefaultParticleShaderID();
	static Graphics::MaterialID GetDefaultMaterialID();

	static Graphics::TextureID LoadTextureAsset(
		GUID guid, const Graphics::TextureParameters &params);
	static Graphics::MeshID	   RequestMesh(std::string_view path);
	static Graphics::MeshID	   RequestMesh(std::string_view name, const Graphics::MeshData &meshData);
	static Graphics::MeshID	   RequestMesh(std::string_view name, std::span<const std::filesystem::path> paths,
										   const Graphics::MeshData &meshData);
	static Graphics::ShaderID  RequestShader(std::string_view name, Graphics::ShaderType type,
											 const std::filesystem::path &vsPath, const std::filesystem::path &psPath);

	static Graphics::MaterialID RequestMaterial(std::string_view name		= DefaultMaterialName,
												GUID			 shaderGuid = DefaultShaderGuid);

	static Graphics::MaterialID RequestMaterial(
		std::string_view																		 name,
		const std::unordered_map<Graphics::MaterialProperty,
								 std::variant<float, int, Math ::Vec2, Math::Vec3, Math::Vec4>> &matProperties,
		std::unordered_map<Graphics::TextureType, std::pair<std::string, std::vector<std::filesystem::path>>>
			&textureBindings,
		GUID shaderGuid = DefaultShaderGuid);
	static Graphics::MaterialID RequestMaterial(const Scene::MaterialConfigBuilder &builder);
	static ModelAssetInfo	   *RequestModel(std::string &modelName, const std::filesystem::path &path,
											 GUID shaderGuid = DefaultShaderGuid);

	static GUID GetTextureGUID(const std::string &path);
	static GUID GetShaderGUID(const std::string &path);
	static GUID GetMaterialGUID(const std::string &path);
	static GUID GetMeshGUID(const std::string &path);
	static GUID GetModelGUID(const std::string &path);

	static Graphics::TextureID	GetTextureHandle(GUID guid);
	static Graphics::ShaderID	GetShaderHandle(GUID guid);
	static Graphics::MaterialID GetMaterialHandle(GUID guid);
	static Graphics::MeshID		GetMeshHandle(GUID guid);

	static TextureAssetInfo*	GetTextureAssetInfo(GUID guid);
	static ShaderAssetInfo*		GetShaderAssetInfo(GUID guid);
	static MaterialAssetInfo*	GetMaterialAssetInfo(GUID guid);
	static MeshAssetInfo*		GetMeshAssetInfo(GUID guid);
	static ModelAssetInfo*		GetModelAssetInfo(GUID guid);

	static const auto &GetTextureGuidToAssetMap() { return s_texGuidToAssetMap; }
	static const auto &GetMeshGuidToAssetMap() { return s_meshGuidToAssetMap; }
	static const auto &GetMaterialGuidToAssetMap() { return s_matGuidToAssetMap; }
	static const auto &GetShaderGuidToAssetMap() { return s_shaderGuidToAssetMap; }
	static const auto &GetModelGuidToAssetMap() { return s_modelGuidToAssetMap; }

	static inline constexpr std::string_view	 DefaultShaderName		   = "Default_Phong_Forward";
	static inline constexpr std::string_view	 DefaultUnlitShaderName	   = "Default_Unlit";
	static inline constexpr std::string_view	 DefaultParticleShaderName = "Default_Particle";
	static inline constexpr std::string_view	 DefaultShadowShaderName   = "Default_Shadow";
	static inline constexpr Graphics::ShaderType DefaultShaderType		   = Graphics::ShaderType::Lit;
	static inline constexpr std::string_view	 DefaultMaterialName	   = "Default_Phong";
	static inline constexpr std::string_view	 DefaultQuadName		   = "Default_Quad";

	static inline GUID DefaultShaderGuid		 = INVALID_GUID;
	static inline GUID DefaultUnlitShaderGuid	 = INVALID_GUID;
	static inline GUID DefaultParticleShaderGuid = INVALID_GUID;
	static inline GUID DefaultShadowShaderGuid	 = INVALID_GUID;
	static inline GUID DefaultMaterialGuid		 = INVALID_GUID;
	static inline GUID DefaultQuadGuid			 = INVALID_GUID;
	static inline GUID ErrorShaderGuid			 = INVALID_GUID;
	static inline GUID ErrorTextureGuid			 = INVALID_GUID;
	static inline GUID ErrorMaterialGuid		 = INVALID_GUID;

	static inline Graphics::ShaderID   DefaultShaderID		   {};
	static inline Graphics::ShaderID   DefaultUnlitShaderID	   {};
	static inline Graphics::ShaderID   DefaultParticleShaderID {};
	static inline Graphics::ShaderID   DefaultShadowShaderID   {};
	static inline Graphics::MaterialID DefaultMaterialID	   {};
	static inline Graphics::MeshID	   DefaultQuadID		   {};

	static inline constexpr std::string_view	 ErrorTextureName  = "Error_Texture";
	static inline constexpr std::string_view	 ErrorShaderName   = "Error_Shader";
	static inline constexpr std::string_view	 ErrorMaterialName = "Error_Material";
	static inline constexpr Graphics::ShaderType ErrorShaderType   = Graphics::ShaderType::Lit;
	static inline Graphics::ShaderID			 ErrorShaderID	   {};
	static inline Graphics::MaterialID			 ErrorMaterialID   {};

private:
	static void ReserveMemory(size_t textureCount, size_t meshCount, size_t materialCount, size_t modelCount,
							  size_t shaderCount);

	template <class T>
	static T *CreateAssetInfoForMemoryAsset(AssetType                              type, std::deque<T> &store,
	                                 std::unordered_map<std::string, GUID> &pathToGuidMap,
	                                 std::unordered_map<GUID, T *> &        guidToAssetMap, std::string_view name);

	template <class T>
	static T *CreateAssetInfo(AssetType type, std::deque<T> &store, std::unordered_map<std::string, GUID> &pathToGuidMap,
	                   const std::filesystem::path &path, std::string_view name);

	template <class T>
	static T *CreateAssetInfo(AssetType type, std::deque<T> &store, std::unordered_map<std::string, GUID> &pathToGuidMap,
	                   std::span<const std::filesystem::path> paths, std::string_view name);

	static void CreateDefaultRenderAssets();
	static void CreateDefaultTextures();
	static void CreateDefaultShaders();
	static void CreateDefaultMaterials();
	static void CreateDefaultMeshes();

	static inline Graphics::IRenderer *ref_renderer = nullptr;
	static inline SystemState		   s_state		= SystemState::Uninitialized;

	static inline std::deque<TextureAssetInfo>	s_textureStore;
	static inline std::deque<MeshAssetInfo>		s_meshStore;
	static inline std::deque<MaterialAssetInfo> s_materialStore;
	static inline std::deque<ShaderAssetInfo>	s_shaderStore;
	static inline std::deque<ModelAssetInfo>	s_modelStore;

	static inline std::unordered_map<std::string, GUID> s_texPathToGuidMap;
	static inline std::unordered_map<std::string, GUID> s_meshPathToGuidMap;
	static inline std::unordered_map<std::string, GUID> s_matPathToGuidMap;
	static inline std::unordered_map<std::string, GUID> s_shaderPathToGuidMap;
	static inline std::unordered_map<std::string, GUID> s_modelPathToGuidMap;

	static inline std::unordered_map<GUID, TextureAssetInfo *>	s_texGuidToAssetMap;
	static inline std::unordered_map<GUID, MeshAssetInfo *>		s_meshGuidToAssetMap;
	static inline std::unordered_map<GUID, MaterialAssetInfo *> s_matGuidToAssetMap;
	static inline std::unordered_map<GUID, ShaderAssetInfo *>	s_shaderGuidToAssetMap;
	static inline std::unordered_map<GUID, ModelAssetInfo *>	s_modelGuidToAssetMap;

	static inline std::unordered_map<Graphics::TextureID, TextureAssetInfo *>	s_texturesById;
	static inline std::unordered_map<Graphics::ShaderID, ShaderAssetInfo *>		s_shadersById;
	static inline std::unordered_map<Graphics::MaterialID, MaterialAssetInfo *> s_materialsById;
	static inline std::unordered_map<Graphics::MeshID, MeshAssetInfo *>			s_meshesById;

	static inline std::vector<GUID> s_defaultTextureGUIDs;
};
}  // namespace PE::Assets