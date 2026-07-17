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
#include "Physics/Body/Components/AABB.h"
#include "Scene/Components/DayNightCycle.h"
#include "Physics/Body/Components/RigidBody.h"
#include "Physics/Body/Components/KinematicBody.h"
#include "Physics/Body/Components/StaticEntity.h"
#include "Physics/Body/Components/BoxCollider.h"
#include "Physics/Body/Components/SphereCollider.h"
#include "Physics/Body/Components/CapsuleCollider.h"
#include "Physics/Body/Components/CylinderCollider.h"
#include "Physics/Body/Components/Aero.h"
#include "Physics/Body/Components/AeroControl.h"
#include "Physics/Body/Components/AngledAero.h"
#include "Physics/Body/Components/Drag.h"
#include "Physics/Body/Components/Buoyancy.h"
#include "Physics/Body/Components/Spring.h"
#include "Physics/Body/Components/AnchoredSpring.h"
#include "Physics/Body/Components/AnchoredBungee.h"
#include "Physics/Body/Components/PhysicsMaterial.h"
#include "Physics/Particle/Components/PointMass.h"
#include "Physics/Particle/Components/Drag.h"
#include "Physics/Particle/Components/Buoyancy.h"
#include "Physics/Particle/Components/Spring.h"
#include "Physics/Particle/Components/AnchoredSpring.h"
#include "Physics/Particle/Components/AnchoredBungee.h"
#include "Physics/Particle/Components/CableConstraint.h"
#include "Physics/Particle/Components/RodConstraint.h"
#include "Scene/Components/Spawner.h"
#include "Scene/Components/WaypointAnimation.h"
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

ERROR_CODE SceneLoader::Initialize(ECS::ECSManager *em, const RenderConfig &config, IRenderer *renderer) {
	ref_eM		 = em;
	ref_config	 = &config;
	ref_renderer = renderer;
	return ERROR_CODE::OK;
}

void SceneLoader::Shutdown() { 
	m_deferredParents.clear(); 
	m_deferredDayNightLinks.clear();
	m_deferredLinks.clear();
	m_materialInteractions.clear();
}

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
			else if (m_currentState == ParseState::MaterialInteraction)
				FinalizeMaterialInteraction();

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
			} else if (type == "MaterialInteraction") {
				m_currentState = ParseState::MaterialInteraction;
				m_matInteractionBuilder = {};
				m_interactionMat1 = 0;
				m_interactionMat2 = 0;
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
			} else if (type == "AABB") {
				m_currentState = ParseState::AABB;
				if (!ref_eM->HasComponent<Physics::Body::Components::AABB>(m_currentEntity)) {
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::AABB{});
				}
			} else if (type == "RigidBody") {
				m_currentState = ParseState::RigidBody;
				if (!ref_eM->HasComponent<Physics::Body::Components::RigidBody>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::RigidBody{});
			} else if (type == "KinematicBody") {
				m_currentState = ParseState::KinematicBody;
				if (!ref_eM->HasComponent<Physics::Body::Components::KinematicBody>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::KinematicBody{});
			} else if (type == "StaticEntity") {
				m_currentState = ParseState::StaticEntity;
				if (!ref_eM->HasComponent<Physics::Body::Components::StaticEntity>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::StaticEntity{});
			} else if (type == "BoxCollider") {
				m_currentState = ParseState::BoxCollider;
				if (!ref_eM->HasComponent<Physics::Body::Components::BoxCollider>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::BoxCollider{});
			} else if (type == "SphereCollider") {
				m_currentState = ParseState::SphereCollider;
				if (!ref_eM->HasComponent<Physics::Body::Components::SphereCollider>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::SphereCollider{});
			} else if (type == "CapsuleCollider") {
				m_currentState = ParseState::CapsuleCollider;
				if (!ref_eM->HasComponent<Physics::Body::Components::CapsuleCollider>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::CapsuleCollider{});
			} else if (type == "CylinderCollider") {
				m_currentState = ParseState::CylinderCollider;
				if (!ref_eM->HasComponent<Physics::Body::Components::CylinderCollider>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::CylinderCollider{});
			} else if (type == "Aero") {
				m_currentState = ParseState::Aero;
				if (!ref_eM->HasComponent<Physics::Body::Components::Aero>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::Aero{});
			} else if (type == "AeroControl") {
				m_currentState = ParseState::AeroControl;
				if (!ref_eM->HasComponent<Physics::Body::Components::AeroControl>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::AeroControl{});
			} else if (type == "AngledAero") {
				m_currentState = ParseState::AngledAero;
				if (!ref_eM->HasComponent<Physics::Body::Components::AngledAero>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::AngledAero{});
			} else if (type == "BodyDrag") {
				m_currentState = ParseState::BodyDrag;
				if (!ref_eM->HasComponent<Physics::Body::Components::Drag>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::Drag{});
			} else if (type == "BodyBuoyancy") {
				m_currentState = ParseState::BodyBuoyancy;
				if (!ref_eM->HasComponent<Physics::Body::Components::Buoyancy>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::Buoyancy{});
			} else if (type == "BodySpring") {
				m_currentState = ParseState::BodySpring;
				if (!ref_eM->HasComponent<Physics::Body::Components::Spring>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::Spring{});
			} else if (type == "BodyAnchoredSpring") {
				m_currentState = ParseState::BodyAnchoredSpring;
				if (!ref_eM->HasComponent<Physics::Body::Components::AnchoredSpring>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::AnchoredSpring{});
			} else if (type == "BodyAnchoredBungee") {
				m_currentState = ParseState::BodyAnchoredBungee;
				if (!ref_eM->HasComponent<Physics::Body::Components::AnchoredBungee>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::AnchoredBungee{});
			} else if (type == "PhysicsMaterial") {
				m_currentState = ParseState::PhysicsMaterial;
				if (!ref_eM->HasComponent<Physics::Body::Components::PhysicsMaterial>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Body::Components::PhysicsMaterial{});
			} else if (type == "PointMass") {
				m_currentState = ParseState::PointMass;
				if (!ref_eM->HasComponent<Physics::Particle::Components::PointMass>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Particle::Components::PointMass{});
			} else if (type == "ParticleDrag") {
				m_currentState = ParseState::ParticleDrag;
				if (!ref_eM->HasComponent<Physics::Particle::Components::Drag>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Particle::Components::Drag{});
			} else if (type == "ParticleBuoyancy") {
				m_currentState = ParseState::ParticleBuoyancy;
				if (!ref_eM->HasComponent<Physics::Particle::Components::Buoyancy>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Particle::Components::Buoyancy{});
			} else if (type == "ParticleSpring") {
				m_currentState = ParseState::ParticleSpring;
				if (!ref_eM->HasComponent<Physics::Particle::Components::Spring>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Particle::Components::Spring{});
			} else if (type == "ParticleAnchoredSpring") {
				m_currentState = ParseState::ParticleAnchoredSpring;
				if (!ref_eM->HasComponent<Physics::Particle::Components::AnchoredSpring>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Particle::Components::AnchoredSpring{});
			} else if (type == "ParticleAnchoredBungee") {
				m_currentState = ParseState::ParticleAnchoredBungee;
				if (!ref_eM->HasComponent<Physics::Particle::Components::AnchoredBungee>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Particle::Components::AnchoredBungee{});
			} else if (type == "CableConstraint") {
				m_currentState = ParseState::CableConstraint;
				if (!ref_eM->HasComponent<Physics::Particle::Components::CableConstraint>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Particle::Components::CableConstraint{});
			} else if (type == "RodConstraint") {
				m_currentState = ParseState::RodConstraint;
				if (!ref_eM->HasComponent<Physics::Particle::Components::RodConstraint>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Physics::Particle::Components::RodConstraint{});
			} else if (type == "Spawner") {
				m_currentState = ParseState::Spawner;
				if (!ref_eM->HasComponent<Components::Spawner>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Components::Spawner{});
			} else if (type == "WaypointAnimation") {
				m_currentState = ParseState::WaypointAnimation;
				if (!ref_eM->HasComponent<Components::WaypointAnimation>(m_currentEntity))
					ref_eM->AddComponent(m_currentEntity, Components::WaypointAnimation{});
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
				case ParseState::MaterialInteraction: HandleMaterialInteractionKey(key, value); break;
				case ParseState::Mesh: HandleMeshKey(key, value); break;
				case ParseState::Entity: HandleEntityKey(key, value); break;
				case ParseState::Tag: HandleTagKey(key, value); break;
				case ParseState::Transform: HandleTransformKey(key, value); break;
				case ParseState::Camera: HandleCameraKey(key, value); break;
				case ParseState::Light: HandleDirectionalLightKey(key, value); break;
				case ParseState::MeshRenderer: HandleMeshRendererKey(key, value); break;
				case ParseState::ParticleEmitter: HandleParticleEmitterKey(key, value); break;
				case ParseState::DayNightCycle: HandleDayNightCycleKey(key, value); break;
				case ParseState::AABB: HandleAABBKey(key, value); break;
				case ParseState::RigidBody: HandleRigidBodyKey(key, value); break;
				case ParseState::KinematicBody: HandleKinematicBodyKey(key, value); break;
				case ParseState::StaticEntity: break; // Tag-only component
				case ParseState::BoxCollider: HandleBoxColliderKey(key, value); break;
				case ParseState::SphereCollider: HandleSphereColliderKey(key, value); break;
				case ParseState::CapsuleCollider: HandleCapsuleColliderKey(key, value); break;
				case ParseState::CylinderCollider: HandleCylinderColliderKey(key, value); break;
				case ParseState::Aero: HandleAeroKey(key, value); break;
				case ParseState::AeroControl: HandleAeroControlKey(key, value); break;
				case ParseState::AngledAero: HandleAngledAeroKey(key, value); break;
				case ParseState::BodyDrag: HandleBodyDragKey(key, value); break;
				case ParseState::BodyBuoyancy: HandleBodyBuoyancyKey(key, value); break;
				case ParseState::BodySpring: HandleBodySpringKey(key, value); break;
				case ParseState::BodyAnchoredSpring: HandleBodyAnchoredSpringKey(key, value); break;
				case ParseState::BodyAnchoredBungee: HandleBodyAnchoredBungeeKey(key, value); break;
				case ParseState::PhysicsMaterial: HandlePhysicsMaterialKey(key, value); break;
				case ParseState::PointMass: HandlePointMassKey(key, value); break;
				case ParseState::ParticleDrag: HandleParticleDragKey(key, value); break;
				case ParseState::ParticleBuoyancy: HandleParticleBuoyancyKey(key, value); break;
				case ParseState::ParticleSpring: HandleParticleSpringKey(key, value); break;
				case ParseState::ParticleAnchoredSpring: HandleParticleAnchoredSpringKey(key, value); break;
				case ParseState::ParticleAnchoredBungee: HandleParticleAnchoredBungeeKey(key, value); break;
				case ParseState::CableConstraint: HandleCableConstraintKey(key, value); break;
				case ParseState::RodConstraint: HandleRodConstraintKey(key, value); break;
				case ParseState::Spawner: HandleSpawnerKey(key, value); break;
				case ParseState::WaypointAnimation: HandleWaypointAnimationKey(key, value); break;
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
	else if (m_currentState == ParseState::MaterialInteraction)
		FinalizeMaterialInteraction();

	FinalizeDayNightCycle();
	FinalizeHierarchy();
	FinalizeLinks();
}

void SceneLoader::ReloadScene() {
	LoadScene(m_lastLoadedScenePath);
}

std::vector<std::string> SceneLoader::SplitString(const std::string &str) {
	std::istringstream		 iss(str);
	std::vector<std::string> results;
	std::string				 token;
	while (iss >> token) results.push_back(token);
	return results;
}

Math::Vec3 SceneLoader::ParseVector3(const std::string &value) {
	std::stringstream ss(value);
	float			  x = 0, y = 0, z = 0;
	ss >> x >> y >> z;
	return Math::Vec3(x, y, z);
}

Math::Vec4 SceneLoader::ParseVector4(const std::string &value) {
	std::stringstream ss(value);
	float			  x = 0, y = 0, z = 0, w = 1.0f;
	ss >> x >> y >> z >> w;
	return Math::Vec4(x, y, z, w);
}

Math::Vec2 SceneLoader::ParseVector2(const std::string &value) {
	std::stringstream ss(value);
	float			  x = 0, y = 0;
	ss >> x >> y;
	return Math::Vec2(x, y);
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

Math::FloatRange SceneLoader::ParseFloatRange(const std::string &value) {
	std::stringstream ss(value);
	float			  min = 0, max = 0;
	ss >> min >> max;
	return {min, max};
}

Math::Vec3Range SceneLoader::ParseVec3Range(const std::string &value) {
	std::stringstream ss(value);
	float			  minX = 0, minY = 0, minZ = 0, maxX = 0, maxY = 0, maxZ = 0;
	ss >> minX >> minY >> minZ >> maxX >> maxY >> maxZ;
	return {{minX, minY, minZ}, {maxX, maxY, maxZ}};
}

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
	else if (key == "HalfHeight")
		m_meshBuilder.height = ParseFloat(value) * 2.0f;
	else if (key == "Depth")
		m_meshBuilder.depth = ParseFloat(value);
	else if (key == "SliceCount")
		m_meshBuilder.sliceCount = ParseInt(value);
	else if (key == "StackCount")
		m_meshBuilder.stackCount = ParseInt(value);
	else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleMaterialInteractionKey(const std::string &key, const std::string &value) {
	if (key == "Mat1") m_interactionMat1 = ParseInt(value);
	else if (key == "Mat2") m_interactionMat2 = ParseInt(value);
	else if (key == "Restitution") m_matInteractionBuilder.restitution = ParseFloat(value);
	else if (key == "StaticFriction") m_matInteractionBuilder.staticFriction = ParseFloat(value);
	else if (key == "DynamicFriction") m_matInteractionBuilder.dynamicFriction = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for MaterialInteraction: " + key);
}

void SceneLoader::HandleEntityKey(const std::string &key, const std::string &value) {
	PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleTagKey(const std::string &key, const std::string &value) {
	if (key == "Name") {
		if (!ref_eM->HasComponent<Components::Tag>(m_currentEntity))
			ref_eM->AddComponent(m_currentEntity, Components::Tag{.name = value});
		else
			ref_eM->GetTComponent<Components::Tag>(m_currentEntity)->name = value;
	} else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleTransformKey(const std::string &key, const std::string &value) {
	auto *tf = ref_eM->GetTComponent<Components::Transform>(m_currentEntity);
	if (key == "Parent") {
		m_deferredParents.push_back({m_currentEntity, value});
	} else if (key == "Position")
		tf->position = ParseVector3(value);
	else if (key == "Rotation")
		tf->orientation = Math::Radians(ParseVector3(value));
	else if (key == "Scale")
		tf->scale = ParseVector3(value);
	else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);

	tf->state = Components::Transform::TransformState::Dirty;
}

void SceneLoader::HandleCameraKey(const std::string &key, const std::string &value) {
	auto *cam = ref_eM->GetTComponent<Graphics::Components::Camera>(m_currentEntity);
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
	auto *l = ref_eM->GetTComponent<Graphics::Components::DirectionalLight>(m_currentEntity);
	if (key == "Color")
		l->color = ParseVector4(value);
	else
		PE_LOG_ERROR("Unknown key-value config pair. Key:" + key + " Value:" + value);
}

void SceneLoader::HandleMeshRendererKey(const std::string &key, const std::string &value) {
	auto *mr = ref_eM->GetTComponent<Graphics::Components::MeshRenderer>(m_currentEntity);
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
	auto *emitter = ref_eM->GetTComponent<Graphics::Components::ParticleEmitter>(m_currentEntity);
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
	auto *comp = ref_eM->GetTComponent<Components::DayNightCycle>(m_currentEntity);
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

void SceneLoader::HandleAABBKey(const std::string &key, const std::string &value) {
	auto *aabb = ref_eM->GetTComponent<Physics::Body::Components::AABB>(m_currentEntity);
	if (!aabb) return;

	if (key == "CollisionLayer") {
		aabb->SetCollisionLayer(ParseInt(value));
	} else if (key == "CollisionMask") {
		aabb->SetCollisionMask(ParseInt(value));
	} else if (key == "ShapeType") {
		if (value == "Box") aabb->SetShapeType(Physics::Body::Components::ColliderShape::Box);
		else if (value == "Sphere") aabb->SetShapeType(Physics::Body::Components::ColliderShape::Sphere);
		else if (value == "Capsule") aabb->SetShapeType(Physics::Body::Components::ColliderShape::Capsule);
		else if (value == "Cylinder") aabb->SetShapeType(Physics::Body::Components::ColliderShape::Cylinder);
		else PE_LOG_ERROR("Unknown ShapeType for AABB: " + value);
	} else if (key == "ColliderType") {
		if (value == "Solid") aabb->SetColliderType(Physics::Body::Components::ColliderType::Solid);
		else if (value == "Container") aabb->SetColliderType(Physics::Body::Components::ColliderType::Container);
		else PE_LOG_ERROR("Unknown ColliderType for AABB: " + value);
	} else { PE_LOG_ERROR("Unknown key for AABB: " + key); }
}

void SceneLoader::HandleRigidBodyKey(const std::string &key, const std::string &value) {
	auto *rb = ref_eM->GetTComponent<Physics::Body::Components::RigidBody>(m_currentEntity);
	if (key == "LinearDamping") rb->linearDamping = ParseFloat(value);
	else if (key == "AngularDamping") rb->angularDamping = ParseFloat(value);
	else if (key == "Mass") rb->SetMass(ParseFloat(value));
	else if (key == "InverseMass") rb->inverseMass = ParseFloat(value);
	else if (key == "GravityScale") rb->gravityScale = ParseFloat(value);
	else if (key == "IsAwake") rb->isAwake = ParseBool(value);
	else if (key == "CanSleep") rb->canSleep = ParseBool(value);
	else PE_LOG_ERROR("Unknown key for RigidBody: " + key);
}

void SceneLoader::HandleKinematicBodyKey(const std::string &key, const std::string &value) {
	auto *kb = ref_eM->GetTComponent<Physics::Body::Components::KinematicBody>(m_currentEntity);
	PE_LOG_ERROR("Unknown key for KinematicBody: " + key);
}

void SceneLoader::HandleBoxColliderKey(const std::string &key, const std::string &value) {
	auto *col = ref_eM->GetTComponent<Physics::Body::Components::BoxCollider>(m_currentEntity);
	if (key == "LocalHalfExtents") col->localHalfExtents = ParseVector3(value);
	else if (key == "LocalOffset") col->localOffset = ParseVector3(value);
	else PE_LOG_ERROR("Unknown key for BoxCollider: " + key);
}

void SceneLoader::HandleSphereColliderKey(const std::string &key, const std::string &value) {
	auto *col = ref_eM->GetTComponent<Physics::Body::Components::SphereCollider>(m_currentEntity);
	if (key == "LocalRadius") col->localRadius = ParseFloat(value);
	else if (key == "LocalOffset") col->localOffset = ParseVector3(value);
	else PE_LOG_ERROR("Unknown key for SphereCollider: " + key);
}

void SceneLoader::HandleCapsuleColliderKey(const std::string &key, const std::string &value) {
	auto *col = ref_eM->GetTComponent<Physics::Body::Components::CapsuleCollider>(m_currentEntity);
	if (key == "LocalRadius") col->localRadius = ParseFloat(value);
	else if (key == "LocalHalfHeight") col->localHalfHeight = ParseFloat(value);
	else if (key == "LocalOffset") col->localOffset = ParseVector3(value);
	else PE_LOG_ERROR("Unknown key for CapsuleCollider: " + key);
}

void SceneLoader::HandleCylinderColliderKey(const std::string &key, const std::string &value) {
	auto *col = ref_eM->GetTComponent<Physics::Body::Components::CylinderCollider>(m_currentEntity);
	if (key == "LocalRadius") col->localRadius = ParseFloat(value);
	else if (key == "LocalHalfHeight") col->localHalfHeight = ParseFloat(value);
	else if (key == "LocalOffset") col->localOffset = ParseVector3(value);
	else PE_LOG_ERROR("Unknown key for CylinderCollider: " + key);
}

void SceneLoader::HandleAeroKey(const std::string &key, const std::string &value) {
	auto *a = ref_eM->GetTComponent<Physics::Body::Components::Aero>(m_currentEntity);
	if (key == "Position") a->position = ParseVector3(value);
	else PE_LOG_ERROR("Unknown key for Aero: " + key);
}

void SceneLoader::HandleAeroControlKey(const std::string &key, const std::string &value) {
	auto *ac = ref_eM->GetTComponent<Physics::Body::Components::AeroControl>(m_currentEntity);
	if (key == "Position") ac->position = ParseVector3(value);
	else if (key == "ControlSetting") ac->controlSetting = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for AeroControl: " + key);
}

void SceneLoader::HandleAngledAeroKey(const std::string &key, const std::string &value) {
	auto *aa = ref_eM->GetTComponent<Physics::Body::Components::AngledAero>(m_currentEntity);
	if (key == "Position") aa->position = ParseVector3(value);
	else if (key == "Orientation") aa->orientation = Math::Quat(Math::Radians(ParseVector3(value)));
	else PE_LOG_ERROR("Unknown key for AngledAero: " + key);
}

void SceneLoader::HandleBodyDragKey(const std::string &key, const std::string &value) {
	auto *d = ref_eM->GetTComponent<Physics::Body::Components::Drag>(m_currentEntity);
	if (key == "K1") d->k1 = ParseFloat(value);
	else if (key == "K2") d->k2 = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for BodyDrag: " + key);
}

void SceneLoader::HandleBodyBuoyancyKey(const std::string &key, const std::string &value) {
	auto *b = ref_eM->GetTComponent<Physics::Body::Components::Buoyancy>(m_currentEntity);
	if (key == "MaxDepth") b->maxDepth = ParseFloat(value);
	else if (key == "Volume") b->volume = ParseFloat(value);
	else if (key == "WaterHeight") b->waterHeight = ParseFloat(value);
	else if (key == "LiquidDensity") b->liquidDensity = ParseFloat(value);
	else if (key == "CentreOfBuoyancy") b->centreOfBuoyancy = ParseVector3(value);
	else PE_LOG_ERROR("Unknown key for BodyBuoyancy: " + key);
}

void SceneLoader::HandleBodySpringKey(const std::string &key, const std::string &value) {
	auto *comp = ref_eM->GetTComponent<Physics::Body::Components::Spring>(m_currentEntity);
	if (key == "OtherRigidBody") m_deferredLinks.push_back({m_currentEntity, value, "", 0});
	else if (key == "SpringConstant") comp->springConstant = ParseFloat(value);
	else if (key == "RestLength") comp->restLength = ParseFloat(value);
	else if (key == "ConnectionPoint") comp->connectionPoint = ParseVector3(value);
	else if (key == "OtherConnectionPoint") comp->otherRbConnectionPoint = ParseVector3(value);
	else PE_LOG_ERROR("Unknown key for BodySpring: " + key);
}

void SceneLoader::HandleBodyAnchoredSpringKey(const std::string &key, const std::string &value) {
	auto *comp = ref_eM->GetTComponent<Physics::Body::Components::AnchoredSpring>(m_currentEntity);
	if (key == "AnchoredRigidBody") m_deferredLinks.push_back({m_currentEntity, value, "", 1});
	else if (key == "SpringConstant") comp->springConstant = ParseFloat(value);
	else if (key == "RestLength") comp->restLength = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for BodyAnchoredSpring: " + key);
}

void SceneLoader::HandleBodyAnchoredBungeeKey(const std::string &key, const std::string &value) {
	auto *comp = ref_eM->GetTComponent<Physics::Body::Components::AnchoredBungee>(m_currentEntity);
	if (key == "OtherRigidBody") m_deferredLinks.push_back({m_currentEntity, value, "", 2});
	else if (key == "SpringConstant") comp->springConstant = ParseFloat(value);
	else if (key == "RestLength") comp->restLength = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for BodyAnchoredBungee: " + key);
}

void SceneLoader::HandlePhysicsMaterialKey(const std::string &key, const std::string &value) {
	auto *pm = ref_eM->GetTComponent<Physics::Body::Components::PhysicsMaterial>(m_currentEntity);
	if (key == "ID") pm->id = static_cast<Physics::Body::PhysicsMaterialID>(ParseInt(value));
	else PE_LOG_ERROR("Unknown key for PhysicsMaterial: " + key);
}

void SceneLoader::HandlePointMassKey(const std::string &key, const std::string &value) {
	auto *pm = ref_eM->GetTComponent<Physics::Particle::Components::PointMass>(m_currentEntity);
	if (key == "LinearDamping") pm->linearDamping = ParseFloat(value);
	else if (key == "Mass") {
		const float mass = ParseFloat(value);
		if (mass != 0) pm->inverseMass = 1.0f / mass;
		else pm->inverseMass = 0.0f;
	}
	else if (key == "InverseMass") pm->inverseMass = ParseFloat(value);
	else if (key == "GravityScale") pm->gravityScale = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for PointMass: " + key);
}

void SceneLoader::HandleParticleDragKey(const std::string &key, const std::string &value) {
	auto *d = ref_eM->GetTComponent<Physics::Particle::Components::Drag>(m_currentEntity);
	if (key == "K1") d->k1 = ParseFloat(value);
	else if (key == "K2") d->k2 = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for ParticleDrag: " + key);
}

void SceneLoader::HandleParticleBuoyancyKey(const std::string &key, const std::string &value) {
	auto *b = ref_eM->GetTComponent<Physics::Particle::Components::Buoyancy>(m_currentEntity);
	if (key == "MaxDepth") b->maxDepth = ParseFloat(value);
	else if (key == "Volume") b->volume = ParseFloat(value);
	else if (key == "WaterHeight") b->waterHeight = ParseFloat(value);
	else if (key == "LiquidDensity") b->liquidDensity = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for ParticleBuoyancy: " + key);
}

void SceneLoader::HandleParticleSpringKey(const std::string &key, const std::string &value) {
	auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::Spring>(m_currentEntity);
	if (key == "OtherPointMass") m_deferredLinks.push_back({m_currentEntity, value, "", 3});
	else if (key == "SpringConstant") comp->springConstant = ParseFloat(value);
	else if (key == "RestLength") comp->restLength = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for ParticleSpring: " + key);
}

void SceneLoader::HandleParticleAnchoredSpringKey(const std::string &key, const std::string &value) {
	auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::AnchoredSpring>(m_currentEntity);
	if (key == "AnchoredPointMass") m_deferredLinks.push_back({m_currentEntity, value, "", 4});
	else if (key == "SpringConstant") comp->springConstant = ParseFloat(value);
	else if (key == "RestLength") comp->restLength = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for ParticleAnchoredSpring: " + key);
}

void SceneLoader::HandleParticleAnchoredBungeeKey(const std::string &key, const std::string &value) {
	auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::AnchoredBungee>(m_currentEntity);
	if (key == "OtherPointMass") m_deferredLinks.push_back({m_currentEntity, value, "", 5});
	else if (key == "SpringConstant") comp->springConstant = ParseFloat(value);
	else if (key == "RestLength") comp->restLength = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for ParticleAnchoredBungee: " + key);
}

void SceneLoader::HandleCableConstraintKey(const std::string &key, const std::string &value) {
	auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::CableConstraint>(m_currentEntity);
	if (key == "PointMasses") {
		auto names = SplitString(value);
		if (names.size() >= 2) m_deferredLinks.push_back({m_currentEntity, names[0], names[1], 6});
	}
	else if (key == "MaxLength") comp->MaxLength = ParseFloat(value);
	else if (key == "Restitution") comp->Restitution = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for CableConstraint: " + key);
}

void SceneLoader::HandleRodConstraintKey(const std::string &key, const std::string &value) {
	auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::RodConstraint>(m_currentEntity);
	if (key == "PointMasses") {
		const std::vector<std::string> names = SplitString(value);
		if (names.size() >= 2) m_deferredLinks.push_back({m_currentEntity, names[0], names[1], 7});
	}
	else if (key == "Length") comp->Length = ParseFloat(value);
	else PE_LOG_ERROR("Unknown key for RodConstraint: " + key);
}

void SceneLoader::HandleSpawnerKey(const std::string &key, const std::string &value) {
	auto *sp = ref_eM->GetTComponent<Components::Spawner>(m_currentEntity);
	if (key == "Name") sp->name = value;
	else if (key == "Shape") {
		if (value == "Box") sp->shape = Physics::Body::Components::ColliderShape::Box;
		else if (value == "Sphere") sp->shape = Physics::Body::Components::ColliderShape::Sphere;
		else if (value == "Capsule") sp->shape = Physics::Body::Components::ColliderShape::Capsule;
		else if (value == "Cylinder") sp->shape = Physics::Body::Components::ColliderShape::Cylinder;
	}
	else if (key == "StartTime") sp->startTime = ParseFloat(value);
	else if (key == "IsSingleBurst") sp->frequency.isSingleBurst = ParseBool(value);
	else if (key == "BurstCount") sp->frequency.burst.count = ParseInt(value);
	else if (key == "RepeatingInterval") sp->frequency.repeating.interval = ParseFloat(value);
	else if (key == "RepeatingMaxCount") sp->frequency.repeating.maxCount = ParseInt(value);
	else if (key == "LocationType") {
		if (value == "FixedLocation") sp->location.type = Components::ULocation::Type::FixedLocation;
		else if (value == "RandomBox") sp->location.type = Components::ULocation::Type::RandomBox;
		else if (value == "RandomSphere") sp->location.type = Components::ULocation::Type::RandomSphere;
	}
	else if (key == "LocationFixedPosition") sp->location.fixed.position = ParseVector3(value);
	else if (key == "LocationFixedOrientationEuler") sp->location.fixed.orientationEuler = ParseVector3(value);
	else if (key == "LocationFixedScale") sp->location.fixed.scale = ParseVector3(value);
	else if (key == "LocationBoxMin") sp->location.box.min = ParseVector3(value);
	else if (key == "LocationBoxMax") sp->location.box.max = ParseVector3(value);
	else if (key == "LocationSphereCenter") sp->location.sphere.center = ParseVector3(value);
	else if (key == "LocationSphereRadius") sp->location.sphere.radius = ParseFloat(value);
	else if (key == "SizeRangeCuboid") sp->sizeRange.cuboid.size = ParseVec3Range(value);
	else if (key == "SizeRangeSphere") sp->sizeRange.sphere.radius = ParseFloatRange(value);
	else if (key == "SizeRangeCapsuleAndCylinderRadius") sp->sizeRange.capsuleAndCylinder.radius = ParseFloatRange(value);
	else if (key == "SizeRangeCapsuleAndCylinderHeight") sp->sizeRange.capsuleAndCylinder.height = ParseFloatRange(value);
	else if (key == "VelocityLinear") sp->velocities.linear = ParseVec3Range(value);
	else if (key == "VelocityAngular") sp->velocities.angular = ParseVec3Range(value);
	else if (key == "Density") sp->density = ParseFloat(value);
	else if (key == "PhysicsMaterialID") sp->physicsMatID = static_cast<Physics::Body::PhysicsMaterialID>(ParseInt(value));
	else if (key == "IsGravityOn") sp->isGravityOn = ParseBool(value);
	else PE_LOG_ERROR("Unknown key for Spawner: " + key);
}

void SceneLoader::HandleWaypointAnimationKey(const std::string &key, const std::string &value) {
	auto *wa = ref_eM->GetTComponent<Components::WaypointAnimation>(m_currentEntity);
	if (key == "StartPos") wa->startPos = ParseVector3(value);
	else if (key == "StartOrientation") wa->startOrientation = Math::Quat(Math::Radians(ParseVector3(value)));
	else if (key == "Duration") wa->duration = ParseFloat(value);
	else if (key == "Waypoint") {
		std::stringstream ss(value);
		float             px;
		float             py;
		float             pz;
		float rx;
		float ry;
		float rz;
		float t;
		if (ss >> px >> py >> pz >> rx >> ry >> rz >> t) {
			wa->waypoints.emplace_back(Math::Vec3(px, py, pz),
				Math::Quat(Math::Radians(Math::Vec3(rx, ry, rz))),
				t);
		} else {
			PE_LOG_ERROR("Invalid Waypoint format. Expected: posX posY posZ rotX rotY rotZ time");
		}
	}
	else if (key == "Easing") {
		if (value == "Linear") wa->easing = Components::WaypointAnimation::EasingType::Linear;
		else if (value == "SmoothStep") wa->easing = Components::WaypointAnimation::EasingType::SmoothStep;
	}
	else if (key == "PathMode") {
		if (value == "Stop") wa->pathMode = Components::WaypointAnimation::PathMode::Stop;
		else if (value == "Loop") wa->pathMode = Components::WaypointAnimation::PathMode::Loop;
		else if (value == "Reverse") wa->pathMode = Components::WaypointAnimation::PathMode::Reverse;
	}
	else PE_LOG_ERROR("Unknown key for WaypointAnimation: " + key);
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
		} else if (m_meshBuilder.shape == "Capsule" || m_meshBuilder.shape == "capsule") {
			GeometryGenerator::CreateCapsule(m_meshBuilder.radius, m_meshBuilder.height,
											  m_meshBuilder.sliceCount, m_meshBuilder.stackCount, data);		}
		else if (m_meshBuilder.shape == "Cylinder" || m_meshBuilder.shape == "cylinder") {
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

void SceneLoader::FinalizeMaterialInteraction() {
	uint32_t matA = std::min(m_interactionMat1, m_interactionMat2);
	uint32_t matB = std::max(m_interactionMat1, m_interactionMat2);

	m_materialInteractions[{matA, matB}] = m_matInteractionBuilder;
	PE_LOG_INFO("Material Interaction Resource Registered for Mat1: " + std::to_string(matA) + " Mat2: " + std::to_string(matB));
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
			auto *childTf			= ref_eM->GetTComponent<Components::Transform>(childID);
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
			if (auto *comp = ref_eM->GetTComponent<Components::DayNightCycle>(link.cycleEntity)) {
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

void SceneLoader::FinalizeLinks() {
	auto &tagArr = ref_eM->GetCompArr<Components::Tag>();
	const auto &tags = tagArr.Data();
	const auto &indices = tagArr.Index();

	for (const auto &link : m_deferredLinks) {
		ECS::EntityID targetID1 = ECS::INVALID_ENTITY_ID;
		ECS::EntityID targetID2 = ECS::INVALID_ENTITY_ID;

		for (size_t i = 0; i < tags.size(); ++i) {
			if (!link.targetName1.empty() && tags[i].name == link.targetName1) targetID1 = indices[i];
			if (!link.targetName2.empty() && tags[i].name == link.targetName2) targetID2 = indices[i];
		}

		switch (link.linkType) {
			case 0: // BodySpring
				if (auto *comp = ref_eM->GetTComponent<Physics::Body::Components::Spring>(link.sourceEntity))
					comp->otherRigidBodyIndex = targetID1;
				break;
			case 1: // BodyAnchoredSpring
				if (auto *comp = ref_eM->GetTComponent<Physics::Body::Components::AnchoredSpring>(link.sourceEntity))
					comp->anchoredRigidBodyIndex = targetID1;
				break;
			case 2: // BodyAnchoredBungee
				if (auto *comp = ref_eM->GetTComponent<Physics::Body::Components::AnchoredBungee>(link.sourceEntity))
					comp->otherRigidBodyIndex = targetID1;
				break;
			case 3: // ParticleSpring
				if (auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::Spring>(link.sourceEntity))
					comp->otherPointMassIndex = targetID1;
				break;
			case 4: // ParticleAnchoredSpring
				if (auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::AnchoredSpring>(link.sourceEntity))
					comp->anchoredPointMassIndex = targetID1;
				break;
			case 5: // ParticleAnchoredBungee
				if (auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::AnchoredBungee>(link.sourceEntity))
					comp->otherPointMassIndex = targetID1;
				break;
			case 6: // CableConstraint
				if (auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::CableConstraint>(link.sourceEntity)) {
					comp->pointMassIndexes[0] = targetID1;
					comp->pointMassIndexes[1] = targetID2;
				}
				break;
			case 7: // RodConstraint
				if (auto *comp = ref_eM->GetTComponent<Physics::Particle::Components::RodConstraint>(link.sourceEntity)) {
					comp->pointMassIndexes[0] = targetID1;
					comp->pointMassIndexes[1] = targetID2;
				}
				break;
		}
	}
	m_deferredLinks.clear();
}
}  // namespace PE::Scene