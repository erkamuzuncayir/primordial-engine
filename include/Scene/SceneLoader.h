#pragma once
#include <map>
#include <string>
#include <vector>

#include "Assets/AssetManager.h"
#include "ECS/ECSManager.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RenderConfig.h"
#include "Physics/Body/Types.h"

namespace PE::Scene {
struct TextureConfigBuilder {
	std::string						   name;
	std::vector<std::filesystem::path> paths;
	Graphics::TextureParameters		   params;
};

struct MeshConfigBuilder {
	std::string			  name;
	std::string			  type;
	std::filesystem::path path;

	std::string shape	   = "Box";
	float		radius	   = 1.0f;
	float		width	   = 1.0f;
	float		height	   = 1.0f;
	float		depth	   = 1.0f;
	int			sliceCount = 32;
	int			stackCount = 32;
};

struct ShaderConfigBuilder {
	std::string			  name;
	Graphics::ShaderType  type = Graphics::ShaderType::Lit;
	std::filesystem::path vsPath;
	std::filesystem::path psPath;
};

struct MaterialConfigBuilder {
	std::string			 name;
	std::string			 shaderName;
	Graphics::MaterialID id;
	std::unordered_map<Graphics::MaterialProperty, std::variant<float, int, Math::Vec2, Math::Vec3, Math::Vec4>>
		matProperties;
	std::unordered_map<Graphics::TextureType, std::pair<std::string, std::vector<std::filesystem::path>>>
																	 textureBindings;
	std::unordered_map<Graphics::TextureType, Graphics::SamplerType> textureSamplerMap;
};

class SceneLoader {
public:
	SceneLoader()  = default;
	~SceneLoader() = default;
	ERROR_CODE Initialize(ECS::ECSManager *em, const Graphics::RenderConfig &config, Graphics::IRenderer *renderer);
	void	   Shutdown();
	void	   LoadScene(const std::string &filePath);
	void	   ReloadScene();

private:
	// Demo specific
	struct DayNightLink {
		ECS::EntityID cycleEntity;
		std::string	  targetName;
		int			  targetType;  // 0:Sun, 1:Moon, 2:Weather
	};

	enum class ParseState {
		None,
		Texture,
		Shader,
		Material,
		Mesh,
		MaterialInteraction,
		Entity,
		Tag,
		Transform,
		Camera,
		Light,
		MeshRenderer,
		ParticleEmitter,
		DayNightCycle,
		AABB,
		RigidBody,
		KinematicBody,
		StaticEntity,
		BoxCollider,
		SphereCollider,
		CapsuleCollider,
		CylinderCollider,
		Aero,
		AeroControl,
		AngledAero,
		BodyDrag,
		BodyBuoyancy,
		BodySpring,
		BodyAnchoredSpring,
		BodyAnchoredBungee,
		PhysicsMaterial,
		PointMass,
		ParticleDrag,
		ParticleBuoyancy,
		ParticleSpring,
		ParticleAnchoredSpring,
		ParticleAnchoredBungee,
		CableConstraint,
		RodConstraint,
		Spawner,
		WaypointAnimation,
	};

	std::vector<std::string> SplitString(const std::string &str);
	Math::Vec3				 ParseVector3(const std::string &value);
	Math::Vec4				 ParseVector4(const std::string &value);
	Math::Vec2				 ParseVector2(const std::string &value);
	float					 ParseFloat(const std::string &value);
	int						 ParseInt(const std::string &value);
	bool					 ParseBool(const std::string &value);
	Math::FloatRange		 ParseFloatRange(const std::string &value);
	Math::Vec3Range			 ParseVec3Range(const std::string &value);

	void HandleTextureKey(const std::string &key, const std::string &value);
	void HandleShaderKey(const std::string &key, const std::string &value);
	void HandleMaterialKey(const std::string &key, const std::string &value);
	void HandleMeshKey(const std::string &key, const std::string &value);
	void HandleMaterialInteractionKey(const std::string &key, const std::string &value);
	void HandleEntityKey(const std::string &key, const std::string &value);
	void HandleTagKey(const std::string &key, const std::string &value);
	void HandleTransformKey(const std::string &key, const std::string &value);
	void HandleCameraKey(const std::string &key, const std::string &value);
	void HandleDirectionalLightKey(const std::string &key, const std::string &value);
	void HandleMeshRendererKey(const std::string &key, const std::string &value);
	void HandleParticleEmitterKey(const std::string &key, const std::string &value);
	void HandleDayNightCycleKey(const std::string &key, const std::string &value);
	void HandleAABBKey(const std::string &key, const std::string &value);
	void HandleRigidBodyKey(const std::string &key, const std::string &value);
	void HandleKinematicBodyKey(const std::string &key, const std::string &value);
	void HandleBoxColliderKey(const std::string &key, const std::string &value);
	void HandleSphereColliderKey(const std::string &key, const std::string &value);
	void HandleCapsuleColliderKey(const std::string &key, const std::string &value);
	void HandleCylinderColliderKey(const std::string &key, const std::string &value);
	void HandleAeroKey(const std::string &key, const std::string &value);
	void HandleAeroControlKey(const std::string &key, const std::string &value);
	void HandleAngledAeroKey(const std::string &key, const std::string &value);
	void HandleBodyDragKey(const std::string &key, const std::string &value);
	void HandleBodyBuoyancyKey(const std::string &key, const std::string &value);
	void HandleBodySpringKey(const std::string &key, const std::string &value);
	void HandleBodyAnchoredSpringKey(const std::string &key, const std::string &value);
	void HandleBodyAnchoredBungeeKey(const std::string &key, const std::string &value);
	void HandlePhysicsMaterialKey(const std::string &key, const std::string &value);
	void HandlePointMassKey(const std::string &key, const std::string &value);
	void HandleParticleDragKey(const std::string &key, const std::string &value);
	void HandleParticleBuoyancyKey(const std::string &key, const std::string &value);
	void HandleParticleSpringKey(const std::string &key, const std::string &value);
	void HandleParticleAnchoredSpringKey(const std::string &key, const std::string &value);
	void HandleParticleAnchoredBungeeKey(const std::string &key, const std::string &value);
	void HandleCableConstraintKey(const std::string &key, const std::string &value);
	void HandleRodConstraintKey(const std::string &key, const std::string &value);
	void HandleSpawnerKey(const std::string &key, const std::string &value);
	void HandleWaypointAnimationKey(const std::string &key, const std::string &value);

	void FinalizeTexture();
	void FinalizeShader();
	void FinalizeMesh();
	void FinalizeMaterial();
	void FinalizeMaterialInteraction();
	void FinalizeHierarchy();
	void FinalizeDayNightCycle();
	void FinalizeLinks();

	ECS::ECSManager				 *ref_eM	   = nullptr;
	const Graphics::RenderConfig *ref_config   = nullptr;
	Graphics::IRenderer			 *ref_renderer = nullptr;

	std::string	  m_lastLoadedScenePath;
	ParseState	  m_currentState  = ParseState::None;
	ECS::EntityID m_currentEntity = ECS::INVALID_ENTITY_ID;
	std::string	  m_currentAssetName;

	TextureConfigBuilder  m_texBuilder;
	MeshConfigBuilder	  m_meshBuilder;
	ShaderConfigBuilder	  m_shaderBuilder;
	MaterialConfigBuilder m_materialBuilder;

	std::vector<std::pair<ECS::EntityID, std::string>> m_deferredParents;
	std::vector<DayNightLink>						   m_deferredDayNightLinks;

	struct DeferredGenericLink {
		ECS::EntityID sourceEntity;
		std::string   targetName1;
		std::string   targetName2;
		int			  linkType;
	};
	std::vector<DeferredGenericLink> m_deferredLinks;

	Physics::Body::MaterialInteraction                                          m_matInteractionBuilder{};
	uint32_t                                                                    m_interactionMat1 = 0;
	uint32_t                                                                    m_interactionMat2 = 0;
	std::map<std::pair<uint32_t, uint32_t>, Physics::Body::MaterialInteraction> m_materialInteractions;
};
}  // namespace PE::Scene