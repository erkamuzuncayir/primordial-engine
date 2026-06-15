#include "Assets/AssetManager.h"

#include <format>

#include "Assets/AssetInfo.h"
#include "Assets/GUID.h"
#include "Assets/Model.h"
#include "Assets/Texture.h"
#include "Assets/Utilities.h"
#include "Common/Common.h"
#include "Core/EngineConfig.h"
#include "Graphics/GeometryGenerator.h"
#include "Graphics/IRenderer.h"
#include "Scene/SceneLoader.h"
#include "Utilities/IOUtilities.h"

namespace PE::Assets {
#if PE_D3D11
static inline const std::filesystem::path DefaultShaderVSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("DiffuseLighting_Forward_vs.cso");
static inline const std::filesystem::path DefaultShaderPSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("DiffuseLighting_Forward_ps.cso");
static inline const std::filesystem::path DefaultUnlitShaderVSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Unlit_vs.cso");
static inline const std::filesystem::path DefaultUnlitShaderPSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Unlit_ps.cso");
static inline const std::filesystem::path DefaultParticleShaderVSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Particle_vs.cso");
static inline const std::filesystem::path DefaultParticleShaderPSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Particle_ps.cso");
static inline const std::filesystem::path DefaultShadowShaderVSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Shadow_vs.cso");
static inline const std::filesystem::path DefaultShadowShaderPSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Shadow_ps.cso");
#elif PE_VULKAN
static inline const std::filesystem::path DefaultShaderVSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Phong_Forward_vert.spv");
static inline const std::filesystem::path DefaultShaderPSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Phong_Forward_frag.spv");
static inline const std::filesystem::path DefaultUnlitShaderVSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Unlit_vert.spv");
static inline const std::filesystem::path DefaultUnlitShaderPSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Unlit_frag.spv");
static inline const std::filesystem::path DefaultParticleShaderVSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Particle_vert.spv");
static inline const std::filesystem::path DefaultParticleShaderPSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Particle_frag.spv");
static inline const std::filesystem::path DefaultShadowShaderVSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Shadow_vert.spv");
static inline const std::filesystem::path DefaultShadowShaderPSPath =
	Utilities::IOUtilities::GetDefaultAssetPath("Default_Shadow_frag.spv");
#endif

ERROR_CODE AssetManager::Initialize(Graphics::IRenderer *renderer, const Core::EngineConfig &engineConfig) {
	PE_CHECK_STATE_INIT(s_state, "AssetManager is already initialized.");
	s_state		 = SystemState::Initializing;
	ref_renderer = renderer;

	const size_t defaultAmount = engineConfig.maxComponentTypeCount;
	ReserveMemory(defaultAmount, defaultAmount, defaultAmount, defaultAmount, defaultAmount);

	CreateDefaultRenderAssets();
	ScanAssets();
	PE_LOG_INFO("AssetManager initialized successfully.");
	s_state = SystemState::Running;
	return ERROR_CODE::OK;
}

ERROR_CODE AssetManager::Shutdown() {
	if (s_state == SystemState::Uninitialized || s_state == SystemState::ShuttingDown) return ERROR_CODE::OK;

	s_state = SystemState::ShuttingDown;

	s_textureStore.clear();
	s_meshStore.clear();
	s_materialStore.clear();
	s_shaderStore.clear();
	s_modelStore.clear();
	s_texPathToGuidMap.clear();
	s_meshPathToGuidMap.clear();
	s_matPathToGuidMap.clear();
	s_shaderPathToGuidMap.clear();
	s_modelPathToGuidMap.clear();
	s_texGuidToAssetMap.clear();
	s_meshGuidToAssetMap.clear();
	s_matGuidToAssetMap.clear();
	s_shaderGuidToAssetMap.clear();
	s_modelGuidToAssetMap.clear();
	s_texturesById.clear();
	s_meshesById.clear();
	s_shadersById.clear();
	s_materialsById.clear();

	ref_renderer = nullptr;
	s_state		 = SystemState::Uninitialized;
	PE_LOG_INFO("AssetManager shutdown complete.");
	return ERROR_CODE::OK;
}

ERROR_CODE AssetManager::RefreshAssetDatabase() {
	// TODO:
	return ERROR_CODE::OK;
}

ERROR_CODE AssetManager::ScanAssets() {
	const std::filesystem::path root = Utilities::IOUtilities::GetAssetsRoot();
	for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
		if (entry.is_regular_file()) {
			const auto path = entry.path();
			const auto filenameStr = path.filename().string();

			std::string ext = path.extension().string();
			std::ranges::transform(ext, ext.begin(), [](const unsigned char c) { return std::tolower(c); });

			if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".hdr" || ext == ".bmp") {
				PE_LOG_INFO(std::format("Discovered Texture: {}", filenameStr));
				CreateAssetInfo(AssetType::Texture, s_textureStore, s_texPathToGuidMap, path, path.stem().c_str());
			}
			else if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae") {
				PE_LOG_INFO(std::format("Discovered Model: {}", filenameStr));
				CreateAssetInfo(AssetType::Model, s_modelStore, s_modelPathToGuidMap, path, path.stem().c_str());
			}
			else if (ext == ".cso" || ext == ".spv" || ext == ".hlsl" || ext == ".glsl" || ext == ".vert" || ext == ".frag") {
				PE_LOG_INFO(std::format("Discovered Shader: {}", filenameStr));
				CreateAssetInfo(AssetType::Shader, s_shaderStore, s_shaderPathToGuidMap, path, path.stem().c_str());
			}
			else if (ext == ".mat" || ext == ".pemat") {
				PE_LOG_INFO(std::format("Discovered Material: {}", filenameStr));
				CreateAssetInfo(AssetType::Material, s_materialStore, s_matPathToGuidMap, path, path.stem().c_str());
			}
			else {
				PE_LOG_WARN(std::format("Unknown file type: {}", path.filename().string()));
			}
		}
	}
	return ERROR_CODE::OK;
}

ERROR_CODE AssetManager::ImportAsset() {
	return ERROR_CODE::OK;
}

Graphics::TextureID AssetManager::GetDefaultTextureIDByType(const Graphics::TextureType type) {
	const size_t typeIndex = static_cast<size_t>(type);

	if (typeIndex >= s_defaultTextureGUIDs.size()) {
		PE_LOG_ERROR("Invalid or out-of-bounds TextureType requested!");
		return {};
	}

	if (const auto handle = GetTextureHandle(s_defaultTextureGUIDs[typeIndex]); handle.IsValid()) {
		return handle;
	}

	PE_LOG_FATAL("Default texture can't found!");
	return {};
}

Graphics::TextureID AssetManager::GetErrorTextureID() { return GetTextureHandle(ErrorTextureGuid); }

Graphics::MeshID AssetManager::CreatePrimitiveMesh(const Graphics::PrimitiveType type, const float radius,
                                                    const float width, const float height, const float depth,
                                                    const int sliceCount, const int stackCount) {
	Graphics::MeshData data;
	std::string		   name;
	switch (type) {
		case Graphics::PrimitiveType::Box:
			Graphics::GeometryGenerator::CreateBox(width, height, depth, data);
			name = std::format("Box_{}W_{}H_{}D", width, height, depth);
			break;
		case Graphics::PrimitiveType::Sphere:
			Graphics::GeometryGenerator::CreateSphere(radius, sliceCount, stackCount, data);
			name = std::format("Sphere_{}R_{}SLC_{}STC", radius, sliceCount, stackCount);
			break;
		case Graphics::PrimitiveType::Geosphere:
			Graphics::GeometryGenerator::CreateGeosphere(radius, sliceCount, data);
			name = std::format("Geosphere_{}R_{}SLC", radius, sliceCount);
			break;
		case Graphics::PrimitiveType::Cylinder:
			Graphics::GeometryGenerator::CreateCylinder(radius, radius, height, sliceCount, stackCount, data);
			name = std::format("Cylinder_{}R_{}R_H{}_{}SLC_{}STC", radius, radius, height, sliceCount, stackCount);
			break;
		case Graphics::PrimitiveType::Grid:
			Graphics::GeometryGenerator::CreateGrid(width, depth, sliceCount, stackCount, data);
			name = std::format("Grid_{}W_{}D_{}SLC_{}STC", width, depth, sliceCount, stackCount);
			break;
		case Graphics::PrimitiveType::FullscreenQuad:
			Graphics::GeometryGenerator::CreateFullscreenQuad(data);
			name = std::format("FullscreenQuad");
			break;
		default: PE_LOG_ERROR("Invalid primitive type!"); break;
	}
	const Graphics::MeshID id = RequestMesh(name, data);
	return id;
}

Graphics::ShaderID AssetManager::GetDefaultShaderID() { return DefaultShaderID; }

Graphics::ShaderID AssetManager::GetDefaultParticleShaderID() { return DefaultParticleShaderID; }

Graphics::MaterialID AssetManager::GetDefaultMaterialID() { return DefaultMaterialID; }

Graphics::TextureID AssetManager::LoadTextureAsset(const GUID guid, const Graphics::TextureParameters			 &params) {
	TextureAssetInfo* assetInfo;
	if (assetInfo = GetTextureAssetInfo(guid); !assetInfo) return {};

	Graphics::Texture tempTex;
	tempTex.SetParameters(params);

	bool loaded = false;
	if (params.isCubemap) {
		loaded = Texture::Loader::LoadCubemap(paths, tempTex);
		if (!loaded) {
			for (auto &path : paths) PE_LOG_ERROR("Failed to load texture: " + path.string());
			return {};
		}
	} else {
		loaded = Texture::Loader::Load(paths[0], tempTex);
		if (!loaded) {
			PE_LOG_ERROR("Failed to load texture: " + paths[0].string());
			return {};
		}
	}

	const Graphics::TextureID id =
		ref_renderer->CreateTexture(texName, tempTex.GetTextureData().data(), tempTex.GetTextureParameters());

	if (id.IsValid()) {
		TextureAssetInfo *newInfo = CreateAssetInfo(AssetType::Texture, s_textureStore, s_texPathToGuidMap, paths, texName);
		newInfo->ref_handle = id;
		newInfo->params		= tempTex.GetTextureParameters();
		s_texturesById[id]	= newInfo;
	}

	return id;
}

Graphics::MeshID AssetManager::RequestMesh(const std::string_view path) {
	if (const Graphics::MeshID handle = GetMeshHandle(PathKeyGenerator(path)); handle.IsValid())
		return handle;
	PE_LOG_WARN(std::format("Mesh not found in registry: {}", path));

	return {};
}

Graphics::MeshID AssetManager::RequestMesh(const std::string_view name, const Graphics::MeshData &meshData) {
	if (const Graphics::MeshID handle = GetMeshHandle(MemoryAssetPathKeyGenerator(name));
		handle.IsValid())
		return handle;

	const Graphics::MeshID id = ref_renderer->CreateMesh(name.data(), meshData);
	if (id.IsValid()) {
		MeshAssetInfo *newInfo = CreateAssetInfoForMemoryAsset(AssetType::Mesh, s_meshStore, s_meshPathToGuidMap, s_meshGuidToAssetMap, name);
		newInfo->name		   = name;
		newInfo->ref_handle	   = id;
		newInfo->vertexCount   = static_cast<uint32_t>(meshData.Vertices.size());
		newInfo->indexCount	   = static_cast<uint32_t>(meshData.Indices.size());
		s_meshesById[id]	   = newInfo;
	}

	return id;
}

Graphics::MeshID AssetManager::RequestMesh(const std::string_view						name,
										   const std::span<const std::filesystem::path> paths,
										   const Graphics::MeshData					   &meshData) {
	if (const Graphics::MeshID handle = GetMeshHandle(PathKeyGenerator(paths)); handle.IsValid())
		return handle;

	const Graphics::MeshID id = ref_renderer->CreateMesh(name.data(), meshData);
	if (id.IsValid()) {
		MeshAssetInfo *newInfo = CreateAssetInfo(AssetType::Mesh, s_meshStore, s_meshPathToGuidMap, paths, name);
		newInfo->ref_handle	   = id;
		newInfo->vertexCount   = static_cast<uint32_t>(meshData.Vertices.size());
		newInfo->indexCount	   = static_cast<uint32_t>(meshData.Indices.size());
		s_meshesById[id]	   = newInfo;
	}

	return id;
}

Graphics::ShaderID AssetManager::RequestShader(const std::string_view name, const Graphics::ShaderType type,
											   const std::filesystem::path &vsPath,
											   const std::filesystem::path &psPath) {
	const Graphics::ShaderID handle = GetShaderHandle(PathKeyGenerator(std::array{vsPath, psPath}));
	if (handle.IsValid()) return handle;

	const Graphics::ShaderID id = ref_renderer->CreateShader(type, vsPath, psPath);

	if (id.IsValid()) {
		const std::filesystem::path paths[] = {vsPath, psPath};
		ShaderAssetInfo			   *newInfo = CreateAssetInfo(AssetType::Shader, s_shaderStore, s_shaderPathToGuidMap, paths, name);
		newInfo->ref_handle = id;
		newInfo->shaderType = type;
		s_shadersById[id]	= newInfo;
	}

	return id;
}
Graphics::MaterialID AssetManager::RequestMaterial(const std::string_view name, const GUID shaderGuid) {
	if (const Graphics::MaterialID handle = GetMaterialHandle(
			name.empty() ? MemoryAssetPathKeyGenerator(DefaultMaterialName) : MemoryAssetPathKeyGenerator(name));
		handle.IsValid())
		return handle;

	const Graphics::ShaderID shaderID = GetShaderHandle(shaderGuid);
	if (!shaderID.IsValid()) {
		PE_LOG_FATAL("Can't find default shader!");
		return {};
	}

	const Graphics::MaterialID matID = ref_renderer->CreateMaterial(shaderID);
	if (matID.IsValid()) {
		MaterialAssetInfo *newInfo =
			CreateAssetInfoForMemoryAsset(AssetType::Material, s_materialStore, s_matPathToGuidMap, s_matGuidToAssetMap, name);
		newInfo->shaderGuid	   = shaderGuid;
		newInfo->ref_handle	   = matID;
		s_materialsById[matID] = newInfo;
	}

	return matID;
}

Graphics::MaterialID AssetManager::RequestMaterial(
	const std::string_view name,
	const std::unordered_map<Graphics::MaterialProperty, std::variant<float, int, Math::Vec2, Math::Vec3, Math::Vec4>>
		&matProperties,
	std::unordered_map<Graphics::TextureType, std::pair<std::string, std::vector<std::filesystem::path>>>
			  &textureBindings, const GUID shaderGuid) {
	if (const Graphics::MaterialID handle = GetMaterialHandle(MemoryAssetPathKeyGenerator(name));
		handle.IsValid())
		return handle;

	const Graphics::ShaderID shaderID = GetShaderHandle(shaderGuid);
	if (!shaderID.IsValid()) return {};

	const Graphics::MaterialID matID = ref_renderer->CreateMaterial(shaderID);
	if (matID.IsValid()) {
		MaterialAssetInfo *newInfo =
			CreateAssetInfoForMemoryAsset(AssetType::Material, s_materialStore, s_matPathToGuidMap, s_matGuidToAssetMap, name);
		newInfo->shaderGuid = shaderGuid;
		newInfo->ref_handle = matID;

		auto &material = ref_renderer->GetMaterial(matID);
		for (auto &[textureType, namePathsPair] : textureBindings) {
			newInfo->textureBindings[textureType] = namePathsPair;
			if (const Graphics::TextureID texID = LoadTextureAsset(TODO, {.type = textureType});
				texID.IsValid()) {
				material.SetTexture(textureType, texID);
			}
		}

		for (const auto &[propKey, propValue] : matProperties) {
			if (std::holds_alternative<float>(propValue)) {
				material.SetProperty(propKey, std::get<float>(propValue));
			} else if (std::holds_alternative<Math::Vec3>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vec3>(propValue));
			} else if (std::holds_alternative<Math::Vec4>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vec4>(propValue));
			} else if (std::holds_alternative<Math::Vec2>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vec2>(propValue));
			}
		}

		ref_renderer->UpdateMaterial(matID);
		for (auto &[type, namePathPair] : textureBindings) {
			ref_renderer->UpdateMaterialTexture(matID, type, GetTextureHandle(PathKeyGenerator(namePathPair.second)));
		}

		s_materialsById[matID] = newInfo;
	}

	return matID;
}

Graphics::MaterialID AssetManager::RequestMaterial(const Scene::MaterialConfigBuilder &builder) {
	if (const Graphics::MaterialID handle = GetMaterialHandle(MemoryAssetPathKeyGenerator(builder.name));
		handle.IsValid())
		return handle;

	GUID shaderGUID;
	Graphics::ShaderID shaderID;
	if (s_shaderPathToGuidMap.contains(builder.shaderPath)) {
		shaderGUID = s_shaderPathToGuidMap[builder.shaderPath];
		shaderID = GetShaderHandle(shaderGUID);
	}
	else {
		PE_LOG_WARN("Shader not found! Using default shader.");
		shaderGUID = DefaultShaderGuid;
		shaderID = GetDefaultShaderID();
	}
	if (!shaderID.IsValid()) return {};

	const Graphics::MaterialID matID = ref_renderer->CreateMaterial(shaderID);
	if (matID.IsValid()) {
		MaterialAssetInfo *newInfo =
			CreateAssetInfoForMemoryAsset(AssetType::Material, s_materialStore, s_matPathToGuidMap, s_matGuidToAssetMap, builder.name);
		newInfo->shaderGuid                                 = shaderGUID;
		newInfo->ref_handle.emplace<Graphics::MaterialID>(matID);

		auto &material = ref_renderer->GetMaterial(matID);
		for (auto &[textureType, namePathPair] : builder.textureBindings) {
			newInfo->textureBindings[textureType] = namePathPair;
			if (const Graphics::TextureID texID =
					LoadTextureAsset(TODO, {.type = textureType});
				texID.IsValid()) {
				material.SetTexture(textureType, texID);
			}
			if (builder.textureSamplerMap.contains(textureType))
				material.SetSampler(textureType, builder.textureSamplerMap.at(textureType));
		}

		for (const auto &[propKey, propValue] : builder.matProperties) {
			if (std::holds_alternative<float>(propValue)) {
				material.SetProperty(propKey, std::get<float>(propValue));
			} else if (std::holds_alternative<Math::Vec3>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vec3>(propValue));
			} else if (std::holds_alternative<Math::Vec4>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vec4>(propValue));
			} else if (std::holds_alternative<Math::Vec2>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vec2>(propValue));
			}
		}

		ref_renderer->UpdateMaterial(matID);
		for (auto &[type, namePathPair] : builder.textureBindings) {
			ref_renderer->UpdateMaterialTexture(matID, type,
												GetTextureHandle(MemoryAssetPathKeyGenerator(namePathPair.first)));
		}

		s_materialsById[matID] = newInfo;
	}

	return matID;
}

ModelAssetInfo *AssetManager::RequestModel(std::string &modelName, const std::filesystem::path &path,
										   const GUID shaderGuid) {
	if (modelName.empty()) modelName = path.stem().string();
	if (ModelAssetInfo *info = GetModelAssetInfo(modelName); info) return info;

	PE_LOG_INFO("Importing Model: " + path.string());
	auto result = Model::Loader::LoadOBJ(path);
	if (!result.success) return nullptr;

	if (const Graphics::ShaderID shaderID = GetShaderHandle(shaderGuid); !shaderID.IsValid()) {
		PE_LOG_FATAL("Nor requested shader or default shader not found for model import.");
		return nullptr;
	}

	if (!result.materials.empty()) {
		for (MaterialAssetInfo &matInfo : result.materials)
			if (!RequestMaterial(matInfo.name, matInfo.properties, matInfo.textureBindings, shaderGuid).IsValid())
				PE_LOG_WARN("Can't load material of model at" + path.string());
	} else {
		for (auto &[meshGuid, matGuid] : result.modelAssetInfo.subMeshes) matGuid = DefaultMaterialGuid;
	}
	for (auto const &[assetInfo, meshData] : result.meshes) {
		if (!RequestMesh(assetInfo.name, assetInfo.paths, meshData).IsValid())
			PE_LOG_WARN("Can't load material of model at" + path.string());
	}

	ModelAssetInfo *newInfo = CreateAssetInfo(AssetType::Model, s_modelStore, s_modelPathToGuidMap, path, modelName);
	newInfo->name			= modelName;
	newInfo->paths.push_back(path);
	newInfo->subMeshes = result.modelAssetInfo.subMeshes;

	return newInfo;
}

GUID AssetManager::GetTextureGUID(const std::string &path) {
	if (s_texPathToGuidMap.contains(path)) return s_texPathToGuidMap[path];
	return INVALID_GUID;
}

GUID AssetManager::GetShaderGUID(const std::string &path) {
	if (s_shaderPathToGuidMap.contains(path)) return s_shaderPathToGuidMap[path];
	return INVALID_GUID;
}

GUID AssetManager::GetMaterialGUID(const std::string &path) {
	if (s_matPathToGuidMap.contains(path)) return s_matPathToGuidMap[path];
	return INVALID_GUID;
}

GUID AssetManager::GetMeshGUID(const std::string &path) {
	if (s_meshPathToGuidMap.contains(path)) return s_meshPathToGuidMap[path];
	return INVALID_GUID;
}

GUID AssetManager::GetModelGUID(const std::string &path) {
	if (s_modelPathToGuidMap.contains(path)) return s_modelPathToGuidMap[path];
	return INVALID_GUID;
}

Graphics::TextureID AssetManager::GetTextureHandle(const GUID guid) {
	if (s_texGuidToAssetMap.contains(guid) && s_texGuidToAssetMap[guid]->IsLoaded()) return std::get<Graphics::TextureID>(s_texGuidToAssetMap[guid]->ref_handle);
	return {};
}

Graphics::ShaderID AssetManager::GetShaderHandle(const GUID guid) {
	if (s_shaderGuidToAssetMap.contains(guid) && s_shaderGuidToAssetMap[guid]->IsLoaded()) return std::get<Graphics::ShaderID>(s_shaderGuidToAssetMap[guid]->ref_handle);
	return {};
}

Graphics::MaterialID AssetManager::GetMaterialHandle(const GUID guid) {
	if (s_matGuidToAssetMap.contains(guid) && s_matGuidToAssetMap[guid]->IsLoaded()) return std::get<Graphics::MaterialID>(s_matGuidToAssetMap[guid]->ref_handle);
	return {};
}

Graphics::MeshID AssetManager::GetMeshHandle(const GUID guid) {
	if (s_meshGuidToAssetMap.contains(guid) && s_meshGuidToAssetMap[guid]->IsLoaded()) return std::get<Graphics::MeshID>(s_meshGuidToAssetMap[guid]->ref_handle);
	return {};
}

TextureAssetInfo * AssetManager::GetTextureAssetInfo(const GUID guid) {
	if (s_texGuidToAssetMap.contains(guid)) return s_texGuidToAssetMap[guid];
	return nullptr;
}

ShaderAssetInfo * AssetManager::GetShaderAssetInfo(const GUID guid) {
	if (s_shaderGuidToAssetMap.contains(guid)) return s_shaderGuidToAssetMap[guid];
	return nullptr;
}

MaterialAssetInfo * AssetManager::GetMaterialAssetInfo(const GUID guid) {
	if (s_matGuidToAssetMap.contains(guid)) return s_matGuidToAssetMap[guid];
	return nullptr;
}

MeshAssetInfo * AssetManager::GetMeshAssetInfo(const GUID guid) {
	if (s_meshGuidToAssetMap.contains(guid)) return s_meshGuidToAssetMap[guid];
	return nullptr;
}

ModelAssetInfo *AssetManager::GetModelAssetInfo(const GUID guid) {
	if (s_modelGuidToAssetMap.contains(guid)) return s_modelGuidToAssetMap[guid];
	return nullptr;
}

void AssetManager::ReserveMemory(size_t textureCount, size_t meshCount, size_t materialCount, size_t modelCount,
								 size_t shaderCount) {
	// TODO: No need now. Change after hot-reload implementation
	// s_textureStore.reserve(textureCount);
	// s_meshStore.reserve(meshCount);
	// s_materialStore.reserve(materialCount);
	// s_shaderStore.reserve(shaderCount);
	// s_modelStore.reserve(modelCount);
	// PE_LOG_INFO("Asset Memory Reserved.");
}

template <typename T>
T *AssetManager::CreateAssetInfoForMemoryAsset(const AssetType type, std::deque<T> &store, std::unordered_map<std::string, GUID> &pathToGuidMap,
									 std::unordered_map<GUID, T *> &guidToAssetMap, std::string_view name) {
	const std::string pathKey = MemoryAssetPathKeyGenerator(name);

	if (pathToGuidMap.contains(pathKey)) {
		PE_LOG_ERROR(std::format("Asset collision! Path already loaded for: {}", name));
		return nullptr;
	}

	T *asset	= &store.emplace_back();
	asset->type = type;
	asset->guid = GUID::Generate();
	asset->name = name;

	pathToGuidMap[pathKey]		= asset->guid;
	guidToAssetMap[asset->guid] = asset;

	return asset;
}

template <typename T>
T *AssetManager::CreateAssetInfo(const AssetType type, std::deque<T> &store, std::unordered_map<std::string, GUID> &pathToGuidMap, const std::filesystem::path &path,
							   std::string_view name) {
	const std::string pathKey = PathKeyGenerator(path);

	if (pathToGuidMap.contains(pathKey)) {
		PE_LOG_ERROR(std::format("Asset collision! Path already loaded for: {}", name));
		return nullptr;
	}

	T *asset	= &store.emplace_back();
	asset->type = type;
	asset->guid = GUID::Generate();
	asset->name = name;

	asset->paths.push_back(path);
	pathToGuidMap[pathKey]		= asset->guid;

	return asset;
}

template <typename T>
T *AssetManager::CreateAssetInfo(const AssetType type, std::deque<T> &store, std::unordered_map<std::string, GUID> &pathToGuidMap,
							   std::span<const std::filesystem::path> paths, std::string_view name) {
	const std::string pathKey = PathKeyGenerator(paths);

	if (pathToGuidMap.contains(pathKey)) {
		PE_LOG_ERROR(std::format("Asset collision! Path already loaded for: {}", name));
		return nullptr;
	}

	T *asset	= &store.emplace_back();
	asset->type = type;
	asset->guid = GUID::Generate();
	asset->name = name;

	asset->paths.assign(paths.begin(), paths.end());

	pathToGuidMap[pathKey]		= asset->guid;

	return asset;
}

void AssetManager::CreateDefaultRenderAssets() {
	CreateDefaultTextures();
	CreateDefaultShaders();
	CreateDefaultMaterials();
	CreateDefaultMeshes();
}

void AssetManager::CreateDefaultTextures() {
	const std::string					  errorTextureName	 = ErrorTextureName.data();
	constexpr Graphics::TextureParameters errorTextureParams = {
		.type = Graphics::TextureType::Albedo, .width = 1, .height = 1};
	const Graphics::TextureID errorId =
		ref_renderer->CreateTexture(errorTextureName, Texture::Generator::GetDefaultError().data(), errorTextureParams);

	if (errorId.IsValid()) {
		TextureAssetInfo *newInfo =
			CreateAssetInfoForMemoryAsset(AssetType::Texture, s_textureStore, s_texPathToGuidMap, s_texGuidToAssetMap, errorTextureName);
		newInfo->ref_handle		= errorId;
		newInfo->params			= errorTextureParams;
		s_texturesById[errorId] = newInfo;
		ErrorTextureGuid		= newInfo->guid;
	}

	constexpr int texTypeCount = static_cast<int>(Graphics::TextureType::Count);
	for (int i = 0; i < texTypeCount; i++) {
		std::string name = "Default_";
		const auto	type = static_cast<Graphics::TextureType>(i);
		name += Utilities::EnumToString(Graphics::TEX_TYPE_MAP, type);
		Graphics::TextureParameters params = {.type = type, .width = 1, .height = 1};
		const Graphics::TextureID	id =
			ref_renderer->CreateTexture(name, Texture::Generator::GetDefaultTexture(type).data(), params);

		if (id.IsValid()) {
			TextureAssetInfo *newInfo =
				CreateAssetInfoForMemoryAsset(AssetType::Texture, s_textureStore, s_texPathToGuidMap, s_texGuidToAssetMap, name);
			newInfo->name = name;
			newInfo->ref_handle = id;
			newInfo->params		= params;
			s_defaultTextureGUIDs.push_back(newInfo->guid);
			s_texturesById[id] = newInfo;
		}
	}
}

void AssetManager::CreateDefaultShaders() {
	ErrorShaderID =
		RequestShader(ErrorShaderName, Graphics::ShaderType::Unlit, DefaultUnlitShaderVSPath, DefaultUnlitShaderPSPath);
	ErrorShaderGuid = s_shadersById[ErrorShaderID]->guid;

	DefaultShaderID =
		RequestShader(DefaultShaderName, Graphics::ShaderType::Lit, DefaultShaderVSPath, DefaultShaderPSPath);
	DefaultShaderGuid = s_shadersById[DefaultShaderID]->guid;

	DefaultUnlitShaderID = RequestShader(DefaultUnlitShaderName, Graphics::ShaderType::Unlit, DefaultUnlitShaderVSPath,
										 DefaultUnlitShaderPSPath);
	DefaultUnlitShaderGuid = s_shadersById[DefaultUnlitShaderID]->guid;

	DefaultShadowShaderID	= RequestShader(DefaultShadowShaderName, Graphics::ShaderType::Shadow,
											DefaultShadowShaderVSPath, DefaultShadowShaderPSPath);
	DefaultShadowShaderGuid = s_shadersById[DefaultShadowShaderID]->guid;

	DefaultParticleShaderID	  = RequestShader(DefaultParticleShaderName, Graphics::ShaderType::Particle,
											  DefaultParticleShaderVSPath, DefaultParticleShaderPSPath);
	DefaultParticleShaderGuid = s_shadersById[DefaultParticleShaderID]->guid;
}

void AssetManager::CreateDefaultMaterials() {
	ErrorMaterialID		= RequestMaterial(ErrorMaterialName, ErrorShaderGuid);
	ErrorMaterialGuid	= s_materialsById[ErrorMaterialID]->guid;
	DefaultMaterialID	= RequestMaterial(DefaultMaterialName, DefaultShaderGuid);
	DefaultMaterialGuid = s_materialsById[DefaultMaterialID]->guid;
}

void AssetManager::CreateDefaultMeshes() {
	Graphics::MeshData meshData;
	Graphics::GeometryGenerator::CreateQuad(1.0, 1.0, meshData);
	DefaultQuadID	= RequestMesh(DefaultQuadName, meshData);
	DefaultQuadGuid = s_meshesById[DefaultQuadID]->guid;
}
}  // namespace PE::Assets