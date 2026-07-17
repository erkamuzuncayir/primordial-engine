#include "Core/Engine.h"

#include "Assets/AssetManager.h"
#include "Graphics/Components/Camera.h"
#include "Graphics/Components/DirectionalLight.h"
#include "Graphics/Components/MeshRenderer.h"
#include "Graphics/Systems/CameraSystem.h"
#include "Physics/Body/Components/Aero.h"
#include "Physics/Body/Components/AeroControl.h"
#include "Physics/Body/Components/AnchoredBungee.h"
#include "Physics/Body/Components/AnchoredSpring.h"
#include "Physics/Body/Components/AngledAero.h"
#include "Physics/Body/Components/Buoyancy.h"
#include "Physics/Body/Components/Drag.h"
#include "Physics/Body/Components/Spring.h"
#include "Physics/Body/Components/StaticEntity.h"
#include "Physics/Particle/Components/AnchoredBungee.h"
#include "Physics/Particle/Components/AnchoredSpring.h"
#include "Physics/Particle/Components/Buoyancy.h"
#include "Physics/Particle/Components/CableConstraint.h"
#include "Physics/Particle/Components/Drag.h"
#include "Physics/Particle/Components/RodConstraint.h"
#include "Physics/Particle/Components/Spring.h"
#include "Scene/Components/Tag.h"
#include "Scene/SceneLoader.h"
#include "Scene/Components/WaypointAnimation.h"
#include "Scene/Systems/DayNightSystem.h"
#include "Utilities/IOUtilities.h"

namespace PE::Core {
ERROR_CODE Engine::Initialize(Platform::PlatformSystem *platformSystem, EngineConfig &config, GLFWwindow *window,
							  Input::InputSystem *inputSystem) {
	PE_CHECK_STATE_INIT(m_state, "Engine is already initialized.");
	m_state = SystemState::Initializing;

	ref_platformSystem = platformSystem;
	ref_inputSystem	   = inputSystem;

	ERROR_CODE result;

	// Initialize Internal
	PE_ENSURE_INIT_SILENT(result, m_ecsManager.Initialize(config));
	PE_ENSURE_INIT_SILENT(result, InitializeComponents(config));
	PE_ENSURE_INIT_SILENT(result, InitializeSystems(config, window));

	// Initialize Assets
	PE_ENSURE_INIT_SILENT(result, Assets::AssetManager::Initialize(m_renderSystem.GetRenderer(), config));
	PE_ENSURE_INIT_SILENT(result, m_renderSystem.GetRenderer()->CreateDefaultResources());

	// Initialize Demo Scene
	PE_ENSURE_INIT_SILENT(
		result, m_sceneLoader.Initialize(&m_ecsManager, config.renderConfig, m_renderSystem.GetRenderer()));
	const std::filesystem::path demoScenePath =
		std::filesystem::path("demo-scenes") / "showcase-colliders.ini";
	m_sceneLoader.LoadScene(Utilities::IOUtilities::GetAssetPath(demoScenePath.string()));
	m_sceneManager.ChangeCamera(0);

	m_state = SystemState::Running;
	return result;
}

ERROR_CODE Engine::InitializeSystems(EngineConfig &config, GLFWwindow *window) {
	ERROR_CODE result;
	// TODO: Remove and move to game logic
	PE_ENSURE_INIT(result, m_dayNightSystem.Initialize(ECS::ESystemStage::Logic, &m_ecsManager),
				   "Scene control system can't initialized.");
	PE_ENSURE_INIT(result,
				   m_transformSystem.Initialize(ECS::ESystemStage::Transform, &m_ecsManager, m_physicsSystem, config),
				   "Transform system can't initialized.");
	PE_ENSURE_INIT(result,
			   m_physicsSystem.Initialize(ECS::ESystemStage::Physics, &m_ecsManager, config, m_transformSystem),
			   "Physics System failed to initialize.");
	PE_ENSURE_INIT(
		result,
		m_cameraSystem.Initialize(ECS::ESystemStage::Camera, &m_ecsManager, ref_inputSystem),
		"Camera system can't initialized.");
	PE_ENSURE_INIT(result,
				   m_renderSystem.Initialize(ECS::ESystemStage::Render, &m_ecsManager, &m_cameraSystem, window, config),
				   "Render system can't initialized.");
	PE_ENSURE_INIT(result,
				   m_particleSystem.Initialize(ECS::ESystemStage::Particle, &m_ecsManager, m_renderSystem.GetRenderer()),
				   "Render system can't initialized.");
	PE_ENSURE_INIT(result,
				   m_guiSystem.Initialize(ECS::ESystemStage::GUI, &m_ecsManager, &m_physicsSystem, &m_sceneManager, &m_sceneLoader,
										  m_renderSystem.GetRenderer()),
				   "GUI System failed to initialize.");
	PE_ENSURE_INIT(
		result,
		m_sceneManager.Initialize(ECS::ESystemStage::SceneManager, this, &m_ecsManager, &m_sceneLoader, ref_inputSystem,
								  &m_transformSystem, &m_physicsSystem, &m_cameraSystem, &m_guiSystem, &m_dayNightSystem, config),
		"Scene control system can't initialized.");

	PE_ENSURE_INIT(
		result,
		m_aabbUpdateSystem.Initialize(ECS::ESystemStage::AABBUpdate, &m_ecsManager, &m_transformSystem, &m_physicsSystem),
		"AABB Update system can't initialized.");

	return result;
}

ERROR_CODE Engine::InitializeComponents(const EngineConfig &config) {
	ERROR_CODE result;
	PE_CHECK(result, m_ecsManager.RegisterComponent<Scene::Components::DayNightCycle>(1));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Scene::Components::Spawner>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Scene::Components::Tag>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Scene::Components::Transform>(config.maxEntityCount));

	PE_CHECK(result, m_ecsManager.RegisterComponent<Graphics::Components::MeshRenderer>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Graphics::Components::Camera>(config.renderConfig.maxCameraCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Graphics::Components::DirectionalLight>(
						 config.renderConfig.maxDirectionalLightCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Graphics::Components::ParticleEmitter>(config.maxEntityCount));

	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::AABB>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::Aero>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::AeroControl>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Body::Components::AnchoredBungee>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Body::Components::AnchoredSpring>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::AngledAero>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::BoxCollider>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::Buoyancy>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Body::Components::CapsuleCollider>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Body::Components::CylinderCollider>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::Drag>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Body::Components::KinematicBody>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Body::Components::PhysicsMaterial>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::RigidBody>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Body::Components::SphereCollider>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::Spring>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Body::Components::StaticEntity>(config.maxEntityCount));

	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Scene::Components::WaypointAnimation>(config.maxEntityCount));

	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Particle::Components::AnchoredBungee>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Particle::Components::AnchoredSpring>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Particle::Components::Buoyancy>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Particle::Components::CableConstraint>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Particle::Components::Drag>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Particle::Components::PointMass>(config.maxEntityCount));
	PE_CHECK(result,
			 m_ecsManager.RegisterComponent<Physics::Particle::Components::RodConstraint>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager.RegisterComponent<Physics::Particle::Components::Spring>(config.maxEntityCount));
	return result;
}

void Engine::UpdateApplication(const float dt) {
	// m_ecsManager.Update(dt);
	m_sceneManager.OnUpdate(dt);
	m_dayNightSystem.OnUpdate(dt);
	m_physicsSystem.OnUpdate(dt);
	m_transformSystem.OnUpdate(dt);
	m_aabbUpdateSystem.OnUpdate(dt);
	m_cameraSystem.OnUpdate(dt);
	m_guiSystem.OnUpdate(dt);
	m_particleSystem.OnUpdate(dt);
	m_renderSystem.OnUpdate(dt);
}

ERROR_CODE Engine::ShutdownComponents() {
	ERROR_CODE result;
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Scene::Components::DayNightCycle>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Scene::Components::Spawner>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Scene::Components::Tag>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Scene::Components::Transform>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Scene::Components::WaypointAnimation>());

	PE_CHECK(result, m_ecsManager.UnregisterComponent<Graphics::Components::MeshRenderer>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Graphics::Components::Camera>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Graphics::Components::DirectionalLight>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Graphics::Components::ParticleEmitter>());

	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::AABB>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::Aero>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::AeroControl>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::AnchoredBungee>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::AnchoredSpring>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::AngledAero>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::BoxCollider>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::Buoyancy>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::CapsuleCollider>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::CylinderCollider>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::Drag>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::KinematicBody>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::PhysicsMaterial>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::RigidBody>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::SphereCollider>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::Spring>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Body::Components::StaticEntity>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Particle::Components::AnchoredBungee>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Particle::Components::AnchoredSpring>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Particle::Components::Buoyancy>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Particle::Components::CableConstraint>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Particle::Components::Drag>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Particle::Components::PointMass>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Particle::Components::RodConstraint>());
	PE_CHECK(result, m_ecsManager.UnregisterComponent<Physics::Particle::Components::Spring>());

	return result;
}

ERROR_CODE Engine::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;

	m_state = SystemState::ShuttingDown;
	m_dayNightSystem.Shutdown();
	m_sceneLoader.Shutdown();
	m_sceneManager.Shutdown();
	m_renderSystem.Shutdown();
	m_particleSystem.Shutdown();
	m_guiSystem.Shutdown();
	m_cameraSystem.Shutdown();
	m_transformSystem.Shutdown();
	m_physicsSystem.Shutdown();
	ERROR_CODE result = ShutdownComponents();
	m_ecsManager.Shutdown();
	Assets::AssetManager::Shutdown();
	m_state = SystemState::Uninitialized;

	return result;
}
}  // namespace PE::Core