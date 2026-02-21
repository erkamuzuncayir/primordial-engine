#include "Scene/SceneLoader.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#include "Assets/Model.h"
#include "Graphics/Components/Camera.h"
#include "Graphics/Components/DirectionalLight.h"
#include "Graphics/Components/MeshRenderer.h"
#include "Graphics/GeometryGenerator.h"
#include "Graphics/Material.h"
#include "Scene/Components/DayNightCycle.h"
#include "Scene/Components/Tag.h"
#include "Scene/Components/Transform.h"
#include "Utilities/IOUtilities.h"
#include "Utilities/Logger.h"
#include "Utilities/StringUtilities.h"

namespace PE::Scene {
namespace Components {
struct DayNightCycle;
}

using namespace PE::Utilities;
using namespace PE::Graphics;

ERROR_CODE SceneLoader::Initialize(ECS::EntityManager *em, const RenderConfig &config, IRenderer *renderer) {
	ref_eM		 = em;
	ref_config	 = &config;
	ref_renderer = renderer;
	return ERROR_CODE::OK;
}

void SceneLoader::Shutdown() { m_deferredParents.clear(); }

void SceneLoader::LoadScene(const std::string &filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		PE_LOG_ERROR("Scene file not found: " + filePath);
		return;
	}

	m_lastLoadedScenePath = filePath;

	std::string line;
	while (std::getline(file, line)) {
		size_t commentPos = line.find(';');
		if (commentPos == std::string::npos) commentPos = line.find("//");
		if (commentPos != std::string::npos) line = line.substr(0, commentPos);

		line = String::Trim(line);
		if (line.empty()) continue;

		if (line.front() == '[' && line.back() == ']') {
			if (m_currentState == ParseState::Texture)
				FinalizeTexture();
			else if (m_currentState == ParseState::Shader)
				FinalizeShader();
			else if (m_currentState == ParseState::Mesh)
				FinalizeMesh();
			else if (m_currentState == ParseState::Material)
				FinalizeMaterial();

			std::string header = line.substr(1, line.size() - 2);

			size_t		colonPos = header.find(':');
			std::string type	 = (colonPos != std::string::npos) ? header.substr(0, colonPos) : header;
			std::string name	 = (colonPos != std::string::npos) ? header.substr(colonPos + 1) : "";

			m_currentAssetName = name;

			if (type == "Texture") {
				m_currentState	  = ParseState::Texture;
				m_texBuilder	  = TextureConfigBuilder();
				m_texBuilder.name = name;
			} else if (type == "Shader") {
				m_currentState		 = ParseState::Shader;
				m_shaderBuilder		 = ShaderConfigBuilder();
				m_shaderBuilder.name = name;
			} else if (type == "Material") {
				m_currentState		   = ParseState::Material;
				m_materialBuilder.name = name;
			} else if (type == "Mesh") {
				m_currentState	   = ParseState::Mesh;
				m_meshBuilder	   = MeshConfigBuilder();
				m_meshBuilder.name = name;
			} else if (type == "Entity") {
				m_currentState	= ParseState::Entity;
				m_currentEntity = ref_eM->CreateEntity();
			} else if (type == "Tag") {
				m_currentState = ParseState::Tag;
				if (!ref_eM->HasComponent<Components::Tag>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Components::Tag{});
			} else if (type == "Transform") {
				m_currentState = ParseState::Transform;
				if (!ref_eM->HasComponent<Components::Transform>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Components::Transform{});
			} else if (type == "Camera") {
				m_currentState = ParseState::Camera;
				if (!ref_eM->HasComponent<Graphics::Components::Camera>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Graphics::Components::Camera{.isDirty = true});
			} else if (type == "DirectionalLight") {
				m_currentState = ParseState::Light;
				if (!ref_eM->HasComponent<Graphics::Components::DirectionalLight>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Graphics::Components::DirectionalLight{});
			} else if (type == "MeshRenderer") {
				m_currentState = ParseState::MeshRenderer;
				if (!ref_eM->HasComponent<Graphics::Components::MeshRenderer>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Graphics::Components::MeshRenderer{});
			} else if (type == "ParticleEmitter") {
				m_currentState = ParseState::ParticleEmitter;
				if (!ref_eM->HasComponent<Graphics::Components::ParticleEmitter>(m_currentEntity)) {
					ref_eM->AddComponent(m_currentEntity, Graphics::Components::ParticleEmitter{});
				}
			} else if (type == "DayNightCycle") {
				m_currentState = ParseState::DayNightCycle;
				if (!ref_eM->HasComponent<Components::DayNightCycle>(m_currentEntity)) {
					ref_eM->AddComponent(m_currentEntity, Components::DayNightCycle{});
				}
			} else
				PE_LOG_ERROR("Unknown type:" + type);

			continue;
		}

		std::istringstream is_line(line);
		std::string		   key, value;
		if (std::getline(is_line, key, '=') && std::getline(is_line, value)) {
			key	  = String::Trim(key);
			value = String::Trim(value);
			if (value.empty()) continue;

			switch (m_currentState) {
				case ParseState::Texture: HandleTextureKey(key, value); break;
				case ParseState::Shader: HandleShaderKey(key, value); break;
				case ParseState::Material: HandleMaterialKey(key, value); break;
				case ParseState::Mesh: HandleMeshKey(key, value); break;
				case ParseState::Entity: HandleEntityKey(key, value); break;
				case ParseState::Tag: HandleTagKey(key, value); break;
				case ParseState::Transform: HandleTransformKey(key, value); break;
				case ParseState::Camera: HandleCameraKey(key, value); break;
				case ParseState::Light: HandleDirectionalLightKey(key, value); break;
				case ParseState::MeshRenderer: HandleMeshRendererKey(key, value); break;
				case ParseState::ParticleEmitter: HandleParticleEmitterKey(key, value); break;
				case ParseState::DayNightCycle: HandleDayNightCycleKey(key, value); break;
				default: PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value); break;
			}
		}
	}

	if (m_currentState == ParseState::Texture)
		FinalizeTexture();
	else if (m_currentState == ParseState::Shader)
		FinalizeShader();
	else if (m_currentState == ParseState::Mesh)
		FinalizeMesh();
	else if (m_currentState == ParseState::Material)
		FinalizeMaterial();

	FinalizeDayNightCycle();
	FinalizeHierarchy();
}

void SceneLoader::ReloadScene() {
	ref_eM->ClearAllEntities();
	LoadScene(m_lastLoadedScenePath);
}

std::vector<std::string> SceneLoader::SplitString(const std::string &str) {
	std::istringstream		 iss(str);
	std::vector<std::string> results;
	std::string				 token;
	while (iss >> token) results.push_back(token);
	return results;
}

Math::Vector3 SceneLoader::ParseVector3(const std::string &value) {
	std::stringstream ss(value);
	float			  x = 0, y = 0, z = 0;
	ss >> x >> y >> z;
	return Math::Vector3(x, y, z);
}

Math::Vector4 SceneLoader::ParseVector4(const std::string &value) {
	std::stringstream ss(value);
	float			  x = 0, y = 0, z = 0, w = 1.0f;
	ss >> x >> y >> z >> w;
	return Math::Vector4(x, y, z, w);
}

Math::Vector2 SceneLoader::ParseVector2(const std::string &value) {
	std::stringstream ss(value);
	float			  x = 0, y = 0;
	ss >> x >> y;
	return Math::Vector2(x, y);
}

float SceneLoader::ParseFloat(const std::string &value) {
	try {
		return std::stof(value);
	} catch (...) {
		return 0.0f;
	}
}

int SceneLoader::ParseInt(const std::string &value) {
	try {
		return std::stoi(value);
	} catch (...) {
		return 0;
	}
}

bool SceneLoader::ParseBool(const std::string &value) { return (value == "true" || value == "1"); }

void SceneLoader::HandleTextureKey(const std::string &key, const std::string &value) {
	if (key == "Type") {
		if (const auto texType = StringToEnum(TEX_TYPE_MAP, value); texType.has_value())
			m_texBuilder.params.type = texType.value();
	} else if (key == "IsCubemap")
		m_texBuilder.params.isCubemap = ParseBool(value);
	else if (key == "Path") {
		m_texBuilder.paths.clear();
		for (const std::vector<std::string> rawPaths = SplitString(value); const auto &splitPath : rawPaths) {
			if (const auto path = std::filesystem::path(splitPath); path.is_absolute())
				m_texBuilder.paths.push_back(splitPath);
			else
				m_texBuilder.paths.push_back(IOUtilities::GetAssetPath(splitPath));
		}
	} else if (key == "Width")
		m_texBuilder.params.width = static_cast<uint16_t>(ParseInt(value));
	else if (key == "Height")
		m_texBuilder.params.height = static_cast<uint16_t>(ParseInt(value));
	else if (key == "Depth")
		m_texBuilder.params.depth = static_cast<uint8_t>(ParseInt(value));
	else if (key == "MipLevels")
		m_texBuilder.params.mipLevels = static_cast<uint8_t>(ParseInt(value));
	else if (key == "ArrayLayers")
		m_texBuilder.params.arrayLayers = static_cast<uint8_t>(ParseInt(value));
	else if (key == "Samples")
		m_texBuilder.params.samples = static_cast<uint8_t>(ParseInt(value));
	else {
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
	}
}

void SceneLoader::HandleShaderKey(const std::string &key, const std::string &value) {
	if (key == "Type") {
		if (value == "Unlit")
			m_shaderBuilder.type = ShaderType::Unlit;
		else if (value == "Lit")
			m_shaderBuilder.type = ShaderType::Lit;
		else if (value == "PBR")
			m_shaderBuilder.type = ShaderType::PBR;
		else if (value == "Terrain")
			m_shaderBuilder.type = ShaderType::Terrain;
		else if (value == "UI")
			m_shaderBuilder.type = ShaderType::UI;
		else if (value == "SnowGlobe")
			m_shaderBuilder.type = ShaderType::SnowGlobe;
		else
			PE_LOG_ERROR("Wrong value for: \"" + key + "\" !");
	} else if (key == "VS_Path") {
		if (const auto path = std::filesystem::path(value); path.is_absolute())
			m_shaderBuilder.vsPath = path;
		else
			m_shaderBuilder.vsPath = IOUtilities::GetAssetPath(value);
	} else if (key == "PS_Path") {
		if (const auto path = std::filesystem::path(value); path.is_absolute())
			m_shaderBuilder.psPath = value;
		else
			m_shaderBuilder.psPath = IOUtilities::GetAssetPath(value);
	} else {
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
	}
}

void SceneLoader::HandleMaterialKey(const std::string &key, const std::string &value) {
	if (key == "Shader") {
		m_materialBuilder.shaderName = value;
	} else if (auto matProp = StringToEnum(MAT_PROP_MAP, key); matProp.has_value()) {
		switch (matProp.value()) {
			case MaterialProperty::Color:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseVector4(value));
				break;
			case MaterialProperty::Tiling:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseVector2(value));
				break;
			case MaterialProperty::Offset:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseVector2(value));
				break;
			case MaterialProperty::SpecularColor:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseVector3(value));
				break;
			case MaterialProperty::SpecularPower:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::Roughness:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::Metallic:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::NormalStrength:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::OcclusionStrength:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::EmissiveColor:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseVector3(value));
				break;
			case MaterialProperty::EmissiveIntensity:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::LayerTiling:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseVector4(value));
				break;
			case MaterialProperty::BlendDistance:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::BlendFalloff:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::Opacity:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::BorderColor:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseVector4(value));
				break;
			case MaterialProperty::BorderThickness:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::Softness:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::Padding:
				m_materialBuilder.matProperties.try_emplace(matProp.value(), ParseFloat(value));
				break;
			case MaterialProperty::Count: PE_LOG_ERROR("Unused property index!"); break;
		}
	} else if (const auto type = StringToEnum(TEX_TYPE_MAP, key); type.has_value()) {
		m_materialBuilder.textureBindings.insert({type.value(), {value, {}}});
	} else if (key.starts_with("Sampler_")) {
		const std::string typeName	  = key.substr(8);
		const std::string samplerName = value;
		if (const auto texType = StringToEnum(TEX_TYPE_MAP, typeName); texType.has_value())
			if (const auto samplerType = StringToEnum(SAMPLER_TYPE_MAP, samplerName); samplerType.has_value())
				m_materialBuilder.textureSamplerMap.insert({texType.value(), samplerType.value()});
	} else {
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
	}
}

void SceneLoader::HandleMeshKey(const std::string &key, const std::string &value) {
	if (key == "Type")
		m_meshBuilder.type = value;
	else if (key == "Path") {
		if (const auto path = std::filesystem::path(value); path.is_absolute())
			m_meshBuilder.path = path;
		else
			m_meshBuilder.path = IOUtilities::GetAssetPath(value);
	} else if (key == "Shape")
		m_meshBuilder.shape = value;
	else if (key == "Radius")
		m_meshBuilder.radius = ParseFloat(value);
	else if (key == "Width")
		m_meshBuilder.width = ParseFloat(value);
	else if (key == "Height")
		m_meshBuilder.height = ParseFloat(value);
	else if (key == "Depth")
		m_meshBuilder.depth = ParseFloat(value);
	else if (key == "SliceCount")
		m_meshBuilder.sliceCount = ParseInt(value);
	else if (key == "StackCount")
		m_meshBuilder.stackCount = ParseInt(value);
	else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleEntityKey(const std::string &key, const std::string &value) {
	PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleTagKey(const std::string &key, const std::string &value) {
	if (key == "Name") {
		if (!ref_eM->HasComponent<Components::Tag>(m_currentEntity))
			ref_eM->AddComponent(m_currentEntity, Components::Tag{.name = value});
		else
			ref_eM->GetTIComponent<Components::Tag>(m_currentEntity)->name = value;
	} else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleTransformKey(const std::string &key, const std::string &value) {
	auto *tf = ref_eM->GetTIComponent<Components::Transform>(m_currentEntity);
	if (key == "Parent") {
		m_deferredParents.push_back({m_currentEntity, value});
	} else if (key == "Position")
		tf->position = ParseVector3(value);
	else if (key == "Rotation")
		tf->rotation = Math::Vec3Radians(ParseVector3(value));
	else if (key == "Scale")
		tf->scale = ParseVector3(value);
	else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);

	tf->state = Components::Transform::TransformState::Dirty;
}

void SceneLoader::HandleCameraKey(const std::string &key, const std::string &value) {
	auto *cam = ref_eM->GetTIComponent<Graphics::Components::Camera>(m_currentEntity);
	if (key == "IsActive")
		cam->isActive = ParseBool(value);
	else if (key == "FOVY")
		cam->fovY = ParseFloat(value);
	else if (key == "NearZ")
		cam->nearZ = ParseFloat(value);
	else if (key == "FarZ")
		cam->farZ = ParseFloat(value);
	else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);

	cam->aspectRatio = static_cast<float>(ref_config->width) / static_cast<float>(ref_config->height);
	cam->isDirty	 = true;
}

void SceneLoader::HandleDirectionalLightKey(const std::string &key, const std::string &value) {
	auto *l = ref_eM->GetTIComponent<Graphics::Components::DirectionalLight>(m_currentEntity);
	if (key == "Color")
		l->color = ParseVector4(value);
	else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleMeshRendererKey(const std::string &key, const std::string &value) {
	auto *mr = ref_eM->GetTIComponent<Graphics::Components::MeshRenderer>(m_currentEntity);
	if (key == "Mesh") {
		if (auto const *modelAssetInfo = Assets::AssetManager::GetModelAssetInfo(value)) {
			mr->subMeshes.clear();
			for (auto &[meshAssetName, materialAssetName] : modelAssetInfo->subMeshes) {
				MaterialID materialHandle = Assets::AssetManager::GetMaterialHandle(materialAssetName);
				if (materialHandle == INVALID_HANDLE) materialHandle = Assets::AssetManager::RequestDefaultMaterial();
				mr->subMeshes.emplace_back(Assets::AssetManager::GetMeshHandle(meshAssetName), materialHandle);
			}
		} else {
			MeshID meshID = Assets::AssetManager::GetMeshHandle(value);
			if (meshID == INVALID_HANDLE) {
				PE_LOG_ERROR("Entity " + std::to_string(m_currentEntity) + " referenced missing mesh: " + value);
				return;
			}

			mr->subMeshes.clear();
			mr->subMeshes.emplace_back();
			mr->subMeshes[0].meshID = meshID;

			mr->subMeshes[0].materialID = Assets::AssetManager::RequestDefaultMaterial();
		}
	} else if (key == "Material") {
		if (const MaterialID matID = Assets::AssetManager::GetMaterialHandle(value); matID != INVALID_HANDLE) {
			for (auto &sm : mr->subMeshes) sm.materialID = matID;
		} else {
			PE_LOG_WARN("Material not found: " + value);
		}
	} else if (key.find("Material/") == 0) {
		std::string idxStr = key.substr(9);
		int			idx	   = ParseInt(idxStr);
		if (idx >= 0 && idx < mr->subMeshes.size()) {
			MaterialID matID = Assets::AssetManager::GetMaterialHandle(value);
			if (matID != INVALID_HANDLE) mr->subMeshes[idx].materialID = matID;
		}
	} else if (key == "IsVisible")
		mr->isVisible = ParseBool(value);
	else if (key == "ForceTransparent")
		mr->forceTransparent = ParseBool(value);
	else if (key == "CastShadows")
		mr->castShadows = ParseBool(value);
	else if (key == "ReceiveShadows")
		mr->receiveShadows = ParseBool(value);
	else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleParticleEmitterKey(const std::string &key, const std::string &value) {
	auto *emitter = ref_eM->GetTIComponent<Graphics::Components::ParticleEmitter>(m_currentEntity);
	if (!emitter) return;

	if (key == "Type") {
		if (const auto type = StringToEnum(PARTICLE_TYPE_MAP, value); type.has_value())
			emitter->type = type.value();
		else
			PE_LOG_ERROR("Invalid Particle Type: " + value);
	} else if (key == "MaxParticles")
		emitter->maxParticles = static_cast<uint32_t>(ParseInt(value));
	else if (key == "SpawnRate")
		emitter->spawnRate = ParseFloat(value);
	else if (key == "LifeTime")
		emitter->lifeTime = ParseFloat(value);
	else if (key == "SpawnRadius")
		emitter->spawnRadius = ParseFloat(value);
	else if (key == "VelocityVar")
		emitter->velocityVar = ParseVector3(value);
	else if (key == "Texture") {
		if (const TextureID texID = Assets::AssetManager::GetTextureHandle(value); texID != INVALID_HANDLE)
			emitter->textureID = texID;
		else
			PE_LOG_WARN("Particle Texture not found: " + value);
	} else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleDayNightCycleKey(const std::string &key, const std::string &value) {
	auto *comp = ref_eM->GetTIComponent<Components::DayNightCycle>(m_currentEntity);
	if (!comp) return;

	if (key == "TimeOfDay")
		comp->timeOfDay = ParseFloat(value);
	else if (key == "DayDuration")
		comp->dayDuration = ParseFloat(value);
	else if (key == "SeasonDuration")
		comp->seasonDuration = ParseFloat(value);
	else if (key == "Sun")
		m_deferredDayNightLinks.emplace_back(m_currentEntity, value, 0);
	else if (key == "Moon")
		m_deferredDayNightLinks.emplace_back(m_currentEntity, value, 1);
	else if (key == "Weather")
		m_deferredDayNightLinks.emplace_back(m_currentEntity, value, 2);
	else if (key == "Dust")
		m_deferredDayNightLinks.emplace_back(m_currentEntity, value, 3);
	else if (key == "Bonfire")
		m_deferredDayNightLinks.emplace_back(m_currentEntity, value, 4);
	else if (key == "RainTexture")
		comp->rainTexture = Assets::AssetManager::GetTextureHandle(value);
	else if (key == "SnowTexture")
		comp->snowTexture = Assets::AssetManager::GetTextureHandle(value);
	else if (key == "DayColor")
		comp->dayColor = ParseVector4(value);
	else if (key == "NightColor")
		comp->nightColor = ParseVector4(value);
	else if (key == "DawnColor")
		comp->dawnColor = ParseVector4(value);
	else if (key == "MoonColor")
		comp->moonColor = ParseVector4(value);
	else {
		PE_LOG_ERROR("Unknown DayNightCycle key: " + key);
	}
}

void SceneLoader::FinalizeTexture() {
	if (!m_texBuilder.name.empty() && !m_texBuilder.paths.empty()) {
		if (const auto id =
				Assets::AssetManager::RequestTexture(m_texBuilder.name, m_texBuilder.paths, m_texBuilder.params);
			id == INVALID_HANDLE) {
			PE_LOG_WARN("Texture Resource Can't Load: " + m_texBuilder.name);
		} else
			PE_LOG_INFO("Texture Resource Loaded: " + m_texBuilder.name);
	}
	m_texBuilder = TextureConfigBuilder();
}

void SceneLoader::FinalizeShader() {
	if (!m_shaderBuilder.name.empty()) {
		if (const auto id = Assets::AssetManager::RequestShader(m_shaderBuilder.name, m_shaderBuilder.type,
																m_shaderBuilder.vsPath, m_shaderBuilder.psPath);
			id == INVALID_HANDLE) {
			PE_LOG_WARN("Shader Resource Can't Load: " + m_shaderBuilder.name);
		} else
			PE_LOG_INFO("Shader Resource Registered: " + m_shaderBuilder.name);
	}
	m_shaderBuilder = ShaderConfigBuilder();
}

void SceneLoader::FinalizeMesh() {
	if (m_meshBuilder.name.empty()) return;

	MeshID meshID = INVALID_HANDLE;

	if (m_meshBuilder.type == "procedural") {
		MeshData data;

		if (m_meshBuilder.shape == "Sphere" || m_meshBuilder.shape == "sphere") {
			GeometryGenerator::CreateSphere(m_meshBuilder.radius, m_meshBuilder.sliceCount, m_meshBuilder.stackCount,
											data);
		} else if (m_meshBuilder.shape == "Box" || m_meshBuilder.shape == "box") {
			GeometryGenerator::CreateBox(m_meshBuilder.width, m_meshBuilder.height, m_meshBuilder.depth, data);
		} else if (m_meshBuilder.shape == "Geosphere" || m_meshBuilder.shape == "geosphere") {
			uint32_t subdiv = std::min(m_meshBuilder.stackCount, 8);
			GeometryGenerator::CreateGeosphere(m_meshBuilder.radius, subdiv, data);
		} else if (m_meshBuilder.shape == "Cylinder" || m_meshBuilder.shape == "cylinder") {
			GeometryGenerator::CreateCylinder(m_meshBuilder.radius, m_meshBuilder.radius, m_meshBuilder.height,
											  m_meshBuilder.sliceCount, m_meshBuilder.stackCount, data);
		} else if (m_meshBuilder.shape == "Grid" || m_meshBuilder.shape == "grid") {
			GeometryGenerator::CreateGrid(m_meshBuilder.width, m_meshBuilder.depth, m_meshBuilder.sliceCount,
										  m_meshBuilder.stackCount, data);
		} else if (m_meshBuilder.shape == "FullscreenQuad") {
			GeometryGenerator::CreateFullscreenQuad(data);
		} else if (m_meshBuilder.shape == "Quad") {
			GeometryGenerator::CreateQuad(m_meshBuilder.width, m_meshBuilder.depth, data);
		} else {
			PE_LOG_ERROR("Invalid mesh shape: " + m_meshBuilder.shape);
		}

		meshID = Assets::AssetManager::RequestMesh(m_meshBuilder.name, data);
		if (meshID == INVALID_HANDLE)
			PE_LOG_WARN("Procedural Mesh can't created!");
		else
			PE_LOG_INFO("Procedural Mesh Created: " + m_meshBuilder.name);
	} else {
		if (const auto modelInfo = Assets::AssetManager::RequestModel(m_meshBuilder.name, m_meshBuilder.path);
			!modelInfo) {
			PE_LOG_WARN("Failed to load model: " + m_meshBuilder.name + " (Path: " + m_meshBuilder.path.string() + ")");
			return;
		} else
			PE_LOG_INFO("OBJ Model Registered: " + modelInfo->name + " (Path: " + modelInfo->sourcePaths[0].string() +
						")");
	}
	m_meshBuilder = MeshConfigBuilder();
}

void SceneLoader::FinalizeMaterial() {
	if (!m_materialBuilder.name.empty()) {
		if (const MaterialID matID = Assets::AssetManager::RequestMaterial(m_materialBuilder); matID == INVALID_HANDLE)
			PE_LOG_WARN("Failed to load material: " + m_materialBuilder.name);
		else
			PE_LOG_INFO("Material Resource Registered: " + m_materialBuilder.name);
	}
	m_materialBuilder = MaterialConfigBuilder();
}

void SceneLoader::FinalizeHierarchy() {
	for (const auto &[childID, parentName] : m_deferredParents) {
		auto		 &tagArr   = ref_eM->GetCompArr<Components::Tag>();
		ECS::EntityID parentID = ECS::INVALID_ENTITY_ID;
		auto		 &tags	   = tagArr.Data();
		for (int i = 0; i < tags.size(); i++) {
			if (auto const &name = tags[i].name; name == parentName) {
				parentID = tagArr.Index()[i];
			}
		}
		if (parentID != ECS::INVALID_ENTITY_ID) {
			auto *childTf			= ref_eM->GetTIComponent<Components::Transform>(childID);
			childTf->parentEntityID = parentID;
			childTf->state			= Components::Transform::TransformState::Dirty;
		} else {
			PE_LOG_WARN("Parent Entity '" + parentName + "' not found for Entity " + std::to_string(childID));
		}
	}
	m_deferredParents.clear();
}

void SceneLoader::FinalizeDayNightCycle() {
	auto	   &tagArr	= ref_eM->GetCompArr<Components::Tag>();
	const auto &tags	= tagArr.Data();
	const auto &indices = tagArr.Index();

	for (const auto &link : m_deferredDayNightLinks) {
		ECS::EntityID targetID = ECS::INVALID_ENTITY_ID;

		for (size_t i = 0; i < tags.size(); ++i) {
			if (tags[i].name == link.targetName) {
				targetID = indices[i];
				break;
			}
		}

		if (targetID != ECS::INVALID_ENTITY_ID) {
			if (auto *comp = ref_eM->GetTIComponent<Components::DayNightCycle>(link.cycleEntity)) {
				if (link.targetType == 0)
					comp->sunEntity = targetID;
				else if (link.targetType == 1)
					comp->moonEntity = targetID;
				else if (link.targetType == 2)
					comp->weatherEntity = targetID;
				else if (link.targetType == 3)
					comp->dustEntity = targetID;
				else if (link.targetType == 4)
					comp->bonfireEntity = targetID;
			}
		} else {
			PE_LOG_WARN("DayNightCycle: Linked entity '" + link.targetName + "' not found!");
		}
	}
	m_deferredDayNightLinks.clear();
}
}  // namespace PE::Scene