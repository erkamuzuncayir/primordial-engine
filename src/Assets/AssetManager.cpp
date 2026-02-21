#include "Assets/AssetManager.h"

#include <format>

#include "Assets/AssetInfo.h"
#include "Assets/Model.h"
#include "Assets/Texture.h"
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
	PE_LOG_INFO("AssetManager initialized successfully.");
	s_state = SystemState::Running;
	return ERROR_CODE::OK;
}

ERROR_CODE AssetManager::Shutdown() {
	if (s_state == SystemState::Uninitialized || s_state == SystemState::ShuttingDown) return ERROR_CODE::OK;

	s_state = SystemState::ShuttingDown;

	s_texAssetRegistry.clear();
	s_meshAssetRegistry.clear();
	s_matAssetRegistry.clear();
	s_shaderAssetRegistry.clear();
	s_modelAssetRegistry.clear();
	s_textureStore.clear();
	s_meshStore.clear();
	s_shaderStore.clear();
	s_materialStore.clear();
	s_texturesById.clear();
	s_meshesById.clear();
	s_shadersById.clear();
	s_materialsById.clear();
	ref_renderer = nullptr;
	s_state		 = SystemState::Uninitialized;
	PE_LOG_INFO("AssetManager shutdown complete.");
	return ERROR_CODE::OK;
}

Graphics::TextureID AssetManager::RequestDefaultTexture(const Graphics::TextureType type) {
	if (const auto handle = GetTextureHandle(s_defaultTextureNames[static_cast<Graphics::TextureID>(type)]);
		Graphics::INVALID_HANDLE != handle)
		return handle;

	PE_LOG_FATAL("Default texture can't found!");
	return Graphics::INVALID_HANDLE;
}

Graphics::MeshID AssetManager::RequestPrimitiveMesh(const Graphics::PrimitiveType type, const float radius,
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

Graphics::ShaderID AssetManager::RequestDefaultShader() { return DefaultShaderID; }

Graphics::ShaderID AssetManager::RequestParticleShader() { return DefaultParticleShaderID; }

Graphics::MaterialID AssetManager::RequestDefaultMaterial() { return DefaultMaterialID; }

Graphics::TextureID AssetManager::RequestTexture(const std::string						  &name,
												 const std::vector<std::filesystem::path> &paths,
												 const Graphics::TextureParameters		  &params) {
	std::string texName = name;
	if (texName.empty()) texName = paths[0].stem().string();

	if (const uint32_t handle = GetTextureHandle(texName); Graphics::INVALID_HANDLE != handle) return handle;

	if (paths.empty()) {
		PE_LOG_ERROR("Path is missing while requesting texture:" + name);
		return Graphics::INVALID_HANDLE;
	}

	Graphics::Texture tempTex;
	tempTex.SetParameters(params);

	bool loaded = false;
	if (params.isCubemap) {
		loaded = Texture::Loader::LoadCubemap(paths, tempTex);
		if (!loaded) {
			for (auto &path : paths) PE_LOG_ERROR("Failed to load texture: " + path.string());
			return Graphics::INVALID_HANDLE;
		}
	} else {
		loaded = Texture::Loader::Load(paths[0], tempTex);
		if (!loaded) {
			PE_LOG_ERROR("Failed to load texture: " + paths[0].string());
			return Graphics::INVALID_HANDLE;
		}
	}

	const Graphics::TextureID id =
		ref_renderer->CreateTexture(texName, tempTex.GetTextureData().data(), tempTex.GetTextureParameters());

	if (id != Graphics::INVALID_HANDLE) {
		TextureAssetInfo *newInfo = AllocateAsset(s_textureStore);
		newInfo->name			  = texName;
		newInfo->sourcePaths	  = paths;
		newInfo->type			  = AssetType::Texture;
		newInfo->ref_handle		  = id;
		newInfo->params			  = tempTex.GetTextureParameters();

		s_texAssetRegistry[texName] = newInfo;
		s_texturesById[id]			= newInfo;
	}

	return id;
}

Graphics::MeshID AssetManager::RequestMesh(const std::string &name) {
	if (const uint32_t handle = GetMeshHandle(name); Graphics::INVALID_HANDLE != handle) return handle;
	PE_LOG_WARN("Mesh not found in registry: " + name);

	return Graphics::INVALID_HANDLE;
}

Graphics::MeshID AssetManager::RequestMesh(const std::string &name, const Graphics::MeshData &meshData) {
	if (const uint32_t handle = GetMeshHandle(name); Graphics::INVALID_HANDLE != handle) return handle;

	const Graphics::MeshID id = ref_renderer->CreateMesh(name, meshData);
	if (id != Graphics::INVALID_HANDLE) {
		MeshAssetInfo *newInfo = AllocateAsset(s_meshStore);
		newInfo->name		   = name;
		newInfo->type		   = AssetType::Mesh;
		newInfo->ref_handle	   = id;
		newInfo->vertexCount   = static_cast<uint32_t>(meshData.Vertices.size());
		newInfo->indexCount	   = static_cast<uint32_t>(meshData.Indices.size());

		s_meshAssetRegistry[name] = newInfo;
		s_meshesById[id]		  = newInfo;
	}

	return id;
}

Graphics::ShaderID AssetManager::RequestShader(const std::string &name, const Graphics::ShaderType type,
											   const std::filesystem::path &vsPath,
											   const std::filesystem::path &psPath) {
	if (const uint32_t handle = GetShaderHandle(name); Graphics::INVALID_HANDLE != handle) return handle;

	const Graphics::ShaderID id = ref_renderer->CreateShader(type, vsPath, psPath);

	if (id != Graphics::INVALID_HANDLE) {
		ShaderAssetInfo *newInfo = AllocateAsset(s_shaderStore);
		newInfo->name			 = name;
		newInfo->type			 = AssetType::Shader;
		newInfo->ref_handle		 = id;
		newInfo->vsPath			 = vsPath;
		newInfo->psPath			 = psPath;
		newInfo->shaderType		 = type;

		s_shaderAssetRegistry[name] = newInfo;
		s_shadersById[id]			= newInfo;
	}

	return id;
}

Graphics::MaterialID AssetManager::RequestMaterial(const std::string &name, const std::string_view &shaderName) {
	if (const Graphics::MaterialID handle = GetMaterialHandle(name.empty() ? DefaultMaterialName.data() : name);
		Graphics::INVALID_HANDLE != handle)
		return handle;

	const Graphics::ShaderID shaderID = GetShaderHandle(shaderName.data());

	if (shaderID == Graphics::INVALID_HANDLE) {
		PE_LOG_FATAL("Can't find default shader!");
		return shaderID;
	}

	const Graphics::MaterialID id = ref_renderer->CreateMaterial(shaderID);
	if (id != Graphics::INVALID_HANDLE) {
		MaterialAssetInfo *newInfo = AllocateAsset(s_materialStore);
		newInfo->name			   = name;
		newInfo->type			   = AssetType::Material;
		newInfo->ref_handle		   = id;

		s_matAssetRegistry[name] = newInfo;
		s_materialsById[id]		 = newInfo;
	}

	return id;
}

Graphics::MaterialID AssetManager::RequestMaterial(
	const std::string																				&name,
	const std::unordered_map<Graphics::MaterialProperty,
							 std::variant<float, int, Math::Vector2, Math::Vector3, Math::Vector4>> &matProperties,
	std::unordered_map<Graphics::TextureType, std::pair<std::string, std::vector<std::filesystem::path>>>
					  &textureBindings,
	const std::string &shaderName) {
	if (const uint32_t handle = GetMaterialHandle(name); Graphics::INVALID_HANDLE != handle) return handle;

	const Graphics::ShaderID shaderID = GetShaderHandle(shaderName);
	if (shaderID == Graphics::INVALID_HANDLE) return Graphics::INVALID_HANDLE;

	const Graphics::MaterialID matID = ref_renderer->CreateMaterial(shaderID);
	if (matID != Graphics::INVALID_HANDLE) {
		MaterialAssetInfo *newInfo = AllocateAsset(s_materialStore);
		newInfo->name			   = name;
		newInfo->shaderAssetName   = shaderName;
		newInfo->type			   = AssetType::Material;
		newInfo->ref_handle		   = matID;

		auto &material = ref_renderer->GetMaterial(matID);
		for (auto &[textureType, namePathsPair] : textureBindings) {
			newInfo->textureBindings[textureType] = namePathsPair;
			if (const Graphics::TextureID texID =
					RequestTexture(namePathsPair.first, namePathsPair.second, {.type = textureType});
				texID != Graphics::INVALID_HANDLE) {
				material.SetTexture(textureType, texID);
			}
		}

		for (const auto &[propKey, propValue] : matProperties) {
			if (std::holds_alternative<float>(propValue)) {
				material.SetProperty(propKey, std::get<float>(propValue));
			} else if (std::holds_alternative<Math::Vector3>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vector3>(propValue));
			} else if (std::holds_alternative<Math::Vector4>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vector4>(propValue));
			} else if (std::holds_alternative<Math::Vector2>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vector2>(propValue));
			}
		}

		ref_renderer->UpdateMaterial(matID);
		for (auto &[type, namePathPair] : textureBindings) {
			ref_renderer->UpdateMaterialTexture(matID, type, GetTextureHandle(namePathPair.first));
		}

		s_matAssetRegistry[name] = newInfo;
		s_materialsById[matID]	 = newInfo;
	}

	return matID;
}

Graphics::MaterialID AssetManager::RequestMaterial(const Scene::MaterialConfigBuilder &builder) {
	if (const uint32_t handle = GetMaterialHandle(builder.name); Graphics::INVALID_HANDLE != handle) return handle;

	const Graphics::ShaderID shaderID = GetShaderHandle(builder.shaderName);
	if (shaderID == Graphics::INVALID_HANDLE) return Graphics::INVALID_HANDLE;

	const Graphics::MaterialID matID = ref_renderer->CreateMaterial(shaderID);
	if (matID != Graphics::INVALID_HANDLE) {
		MaterialAssetInfo *newInfo = AllocateAsset(s_materialStore);
		newInfo->name			   = builder.name;
		newInfo->shaderAssetName   = builder.shaderName;
		newInfo->type			   = AssetType::Material;
		newInfo->ref_handle		   = matID;

		auto &material = ref_renderer->GetMaterial(matID);
		for (auto &[textureType, namePathPair] : builder.textureBindings) {
			newInfo->textureBindings[textureType] = namePathPair;
			if (const Graphics::TextureID texID =
					RequestTexture(namePathPair.first, namePathPair.second, {.type = textureType});
				texID != Graphics::INVALID_HANDLE) {
				material.SetTexture(textureType, texID);
			}
			if (builder.textureSamplerMap.contains(textureType))
				material.SetSampler(textureType, builder.textureSamplerMap.at(textureType));
		}

		for (const auto &[propKey, propValue] : builder.matProperties) {
			if (std::holds_alternative<float>(propValue)) {
				material.SetProperty(propKey, std::get<float>(propValue));
			} else if (std::holds_alternative<Math::Vector3>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vector3>(propValue));
			} else if (std::holds_alternative<Math::Vector4>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vector4>(propValue));
			} else if (std::holds_alternative<Math::Vector2>(propValue)) {
				material.SetProperty(propKey, std::get<Math::Vector2>(propValue));
			}
		}

		ref_renderer->UpdateMaterial(matID);
		for (auto &[type, namePathPair] : builder.textureBindings) {
			ref_renderer->UpdateMaterialTexture(matID, type, GetTextureHandle(namePathPair.first));
		}

		s_matAssetRegistry[builder.name] = newInfo;
		s_materialsById[matID]			 = newInfo;
	}

	return matID;
}

ModelAssetInfo *AssetManager::RequestModel(std::string &modelName, const std::filesystem::path &path,
										   const std::string &shaderName) {
	if (modelName.empty()) modelName = path.stem().string();
	if (ModelAssetInfo *info = GetModelAssetInfo(modelName); info) return info;

	PE_LOG_INFO("Importing Model: " + path.string());
	auto result = Model::Loader::LoadOBJ(path);
	if (!result.success) return nullptr;

	if (const Graphics::ShaderID shaderID = GetShaderHandle(shaderName); shaderID == Graphics::INVALID_HANDLE) {
		PE_LOG_FATAL("Nor requested shader or default shader not found for model import.");
		return nullptr;
	}

	if (!result.materials.empty()) {
		for (MaterialAssetInfo &matInfo : result.materials)
			if (RequestMaterial(matInfo.name, matInfo.properties, matInfo.textureBindings, shaderName) ==
				Graphics::INVALID_HANDLE)
				PE_LOG_WARN("Can't load material of model at" + path.string());
	} else {
		RequestMaterial(DefaultMaterialName.data(), shaderName);
		for (auto &[meshAssetName, materialAssetName] : result.modelAssetInfo.subMeshes)
			materialAssetName = DefaultMaterialName;
	}
	for (auto const &[assetInfo, meshData] : result.meshes) {
		if (RequestMesh(assetInfo.name, meshData) == Graphics::INVALID_HANDLE)
			PE_LOG_WARN("Can't load material of model at" + path.string());
	}

	ModelAssetInfo *newInfo = AllocateAsset(s_modelStore);
	newInfo->name			= modelName;
	newInfo->type			= AssetType::Model;
	newInfo->sourcePaths.push_back(path);
	newInfo->subMeshes = result.modelAssetInfo.subMeshes;

	s_modelAssetRegistry[modelName] = newInfo;

	return newInfo;
}

Graphics::TextureID AssetManager::GetTextureHandle(const std::string &fileName) {
	if (const auto it = s_texAssetRegistry.find(fileName); it != s_texAssetRegistry.end()) {
		return it->second->ref_handle;
	}
	return Graphics::INVALID_HANDLE;
}

Graphics::ShaderID AssetManager::GetShaderHandle(const std::string &fileName) {
	if (const auto it = s_shaderAssetRegistry.find(fileName); it != s_shaderAssetRegistry.end()) {
		return it->second->ref_handle;
	}
	return Graphics::INVALID_HANDLE;
}

Graphics::MaterialID AssetManager::GetMaterialHandle(const std::string &fileName) {
	if (const auto it = s_matAssetRegistry.find(fileName); it != s_matAssetRegistry.end()) {
		return it->second->ref_handle;
	}
	return Graphics::INVALID_HANDLE;
}

Graphics::MeshID AssetManager::GetMeshHandle(const std::string &fileName) {
	if (const auto it = s_meshAssetRegistry.find(fileName); it != s_meshAssetRegistry.end()) {
		return it->second->ref_handle;
	}
	return Graphics::INVALID_HANDLE;
}

ModelAssetInfo *AssetManager::GetModelAssetInfo(const std::string &fileName) {
	if (const auto it = s_modelAssetRegistry.find(fileName); it != s_modelAssetRegistry.end()) {
		return it->second;
	}
	return nullptr;
}

void AssetManager::ReserveMemory(size_t textureCount, size_t meshCount, size_t materialCount, size_t modelCount,
								 size_t shaderCount) {
	s_textureStore.reserve(textureCount);
	s_meshStore.reserve(meshCount);
	s_materialStore.reserve(materialCount);
	s_shaderStore.reserve(shaderCount);
	s_modelStore.reserve(modelCount);
	PE_LOG_INFO("Asset Memory Reserved.");
}

template <typename T>
T *AssetManager::AllocateAsset(std::vector<T> &store) {
	if (store.size() >= store.capacity()) {
		PE_LOG_WARN("AssetStore capacity exceeded!");
	}
	store.emplace_back();
	return &store.back();
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

	if (errorId != Graphics::INVALID_HANDLE) {
		TextureAssetInfo *newInfo = AllocateAsset(s_textureStore);
		newInfo->name			  = errorTextureName;

		newInfo->type		= AssetType::Texture;
		newInfo->ref_handle = errorId;
		newInfo->params		= errorTextureParams;

		s_texAssetRegistry[errorTextureName] = newInfo;
		s_texturesById[errorId]				 = newInfo;
	}

	constexpr int texTypeCount = static_cast<int>(Graphics::TextureType::Count);
	for (int i = 0; i < texTypeCount; i++) {
		std::string name = "Default_";
		const auto	type = static_cast<Graphics::TextureType>(i);
		name += Utilities::EnumToString(Graphics::TEX_TYPE_MAP, type);
		Graphics::TextureParameters params = {.type = type, .width = 1, .height = 1};
		const Graphics::TextureID	id =
			ref_renderer->CreateTexture(name, Texture::Generator::GetDefaultTexture(type).data(), params);

		if (id != Graphics::INVALID_HANDLE) {
			TextureAssetInfo *newInfo = AllocateAsset(s_textureStore);
			newInfo->name			  = name;

			newInfo->type		= AssetType::Texture;
			newInfo->ref_handle = id;
			newInfo->params		= params;

			s_defaultTextureNames.push_back(name);
			s_texAssetRegistry[name] = newInfo;
			s_texturesById[id]		 = newInfo;
		}
	}
}

void AssetManager::CreateDefaultShaders() {
	ErrorShaderID = RequestShader(ErrorShaderName.data(), Graphics::ShaderType::Unlit, DefaultUnlitShaderVSPath,
								  DefaultUnlitShaderPSPath);
	DefaultShaderID =
		RequestShader(DefaultShaderName.data(), Graphics::ShaderType::Lit, DefaultShaderVSPath, DefaultShaderPSPath);
	DefaultUnlitShaderID	= RequestShader(DefaultUnlitShaderName.data(), Graphics::ShaderType::Unlit,
											DefaultUnlitShaderVSPath, DefaultUnlitShaderPSPath);
	DefaultShadowShaderID	= RequestShader(DefaultShadowShaderName.data(), Graphics::ShaderType::Shadow,
											DefaultShadowShaderVSPath, DefaultShadowShaderPSPath);
	DefaultParticleShaderID = RequestShader(DefaultParticleShaderName.data(), Graphics::ShaderType::Particle,
											DefaultParticleShaderVSPath, DefaultParticleShaderPSPath);
}

void AssetManager::CreateDefaultMaterials() {
	ErrorMaterialID	  = RequestMaterial(ErrorMaterialName.data(), ErrorShaderName);
	DefaultMaterialID = RequestMaterial(DefaultMaterialName.data(), DefaultShaderName);
}

void AssetManager::CreateDefaultMeshes() {
	Graphics::MeshData meshData;
	Graphics::GeometryGenerator::CreateQuad(1.0, 1.0, meshData);
	DefaultQuadID = RequestMesh(DefaultQuadName.data(), meshData);
}
}  // namespace PE::Assets