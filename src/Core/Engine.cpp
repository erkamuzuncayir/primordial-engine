#include "Core/Engine.h"

#include "Assets/AssetManager.h"
#include "Graphics/Components/Camera.h"
#include "Graphics/Components/DirectionalLight.h"
#include "Graphics/Components/MeshRenderer.h"
#include "Graphics/Systems/CameraSystem.h"
#include "Scene/Components/Tag.h"
#include "Scene/SceneLoader.h"
#include "Scene/Systems/DayNightSystem.h"
#include "Utilities/IOUtilities.h"
#include "Utilities/MemoryUtilities.h"

namespace PE::Core {
ERROR_CODE Engine::Initialize(Platform::PlatformSystem *platformSystem, EngineConfig &config, GLFWwindow *window,
							  Input::InputSystem *inputSystem) {
	PE_CHECK_STATE_INIT(m_state, "Engine is already initialized.");
	m_state = SystemState::Initializing;

	ref_platformSystem = platformSystem;
	ref_inputSystem	   = inputSystem;

	ERROR_CODE result;
	m_entityManager = new ECS::EntityManager();
	m_sceneLoader	= new Scene::SceneLoader();

	// Initialize Internal
	PE_ENSURE_INIT_SILENT(result, m_entityManager->Initialize(config.maxEntityCount, config.maxComponentTypeCount));
	PE_ENSURE_INIT_SILENT(result, InitializeComponents(config));
	PE_ENSURE_INIT_SILENT(result, InitializeSystems(config, window));

	// Initialize Assets
	PE_ENSURE_INIT_SILENT(result, Assets::AssetManager::Initialize(m_renderSystem->GetRenderer(), config));
	PE_ENSURE_INIT_SILENT(result, m_renderSystem->GetRenderer()->CreateDefaultResources());

	// Initialize Demo Scene
	PE_ENSURE_INIT_SILENT(
		result, m_sceneLoader->Initialize(m_entityManager, config.renderConfig, m_renderSystem->GetRenderer()));
	const std::filesystem::path demoScenePath =
		std::filesystem::path("demo-scenes") / "desert-globe" / "desert-globe.ini";
	m_sceneLoader->LoadScene(Utilities::IOUtilities::GetAssetPath(demoScenePath.string()));
	m_sceneControlSystem->SelectControlledEntity(Graphics::Systems::CameraType::Overview);

	m_state = SystemState::Running;
	return result;
}

ERROR_CODE Engine::InitializeSystems(EngineConfig &config, GLFWwindow *window) {
	ERROR_CODE result;
	m_transformSystem = new Scene::Systems::TransformSystem();
	PE_ENSURE_INIT(result,
				   m_transformSystem->Initialize(ECS::ESystemStage::Transform, m_entityManager, ref_inputSystem,
												 m_cameraSystem, config),
				   "Transform system can't initialized.");

	m_cameraSystem = new Graphics::Systems::CameraSystem();
	PE_ENSURE_INIT(
		result,
		m_cameraSystem->Initialize(ECS::ESystemStage::Camera, m_entityManager, ref_inputSystem, config.renderConfig),
		"Camera system can't initialized.");
	m_renderSystem = new Graphics::Systems::RenderSystem();
	PE_ENSURE_INIT(
		result, m_renderSystem->Initialize(ECS::ESystemStage::Render, m_entityManager, m_cameraSystem, window, config),
		"Render system can't initialized.");
	m_particleSystem = new Graphics::Systems::ParticleSystem();
	PE_ENSURE_INIT(
		result,
		m_particleSystem->Initialize(ECS::ESystemStage::Particle, m_entityManager, m_renderSystem->GetRenderer()),
		"Render system can't initialized.");
	m_guiSystem = new Graphics::Systems::GUISystem();
	PE_ENSURE_INIT(result,
				   m_guiSystem->Initialize(ECS::ESystemStage::GUI, m_entityManager, m_sceneControlSystem,
										   m_renderSystem->GetRenderer()),
				   "GUI System failed to initialize.");
	m_dayNightSystem = new Scene::Systems::DayNightSystem();
	PE_ENSURE_INIT(result, m_dayNightSystem->Initialize(ECS::ESystemStage::GameLogic, m_entityManager),
				   "Scene control system can't initialized.");
	m_sceneControlSystem = new Scene::Systems::SceneControlSystem();
	PE_ENSURE_INIT(result,
				   m_sceneControlSystem->Initialize(ECS::ESystemStage::SceneControl, this, m_entityManager,
													m_sceneLoader, ref_inputSystem, m_transformSystem, m_cameraSystem,
													m_guiSystem, m_dayNightSystem, config),
				   "Scene control system can't initialized.");
	return result;
}

ERROR_CODE Engine::InitializeComponents(const EngineConfig &config) const {
	ERROR_CODE result;
	PE_CHECK(result, m_entityManager->RegisterComponent<Scene::Components::Tag>(config.maxEntityCount));
	PE_CHECK(result, m_entityManager->RegisterComponent<Scene::Components::Transform>(config.maxEntityCount));
	PE_CHECK(result, m_entityManager->RegisterComponent<Graphics::Components::MeshRenderer>(config.maxEntityCount));
	PE_CHECK(result,
			 m_entityManager->RegisterComponent<Graphics::Components::Camera>(config.renderConfig.maxCameraCount));
	PE_CHECK(result, m_entityManager->RegisterComponent<Graphics::Components::DirectionalLight>(
						 config.renderConfig.maxDirectionalLightCount));
	PE_CHECK(result, m_entityManager->RegisterComponent<Graphics::Components::ParticleEmitter>(config.maxEntityCount));
	PE_CHECK(result, m_entityManager->RegisterComponent<Scene::Components::DayNightCycle>(1));
	return result;
}

void Engine::UpdateApplication(const float dt) {
	static float totalTime = 0.0f;

	m_sceneControlSystem->OnUpdate(dt);
	m_dayNightSystem->OnUpdate(dt);
	m_transformSystem->OnUpdate(dt);
	m_cameraSystem->OnUpdate(dt);
	m_guiSystem->OnUpdate(dt);
	m_particleSystem->OnUpdate(dt);
	m_renderSystem->OnUpdate(dt);

	totalTime += dt;
}

ERROR_CODE Engine::ShutdownComponents() const {
	ERROR_CODE result;
	PE_CHECK(result, m_entityManager->UnregisterComponent<Graphics::Components::DirectionalLight>());
	PE_CHECK(result, m_entityManager->UnregisterComponent<Graphics::Components::Camera>());
	PE_CHECK(result, m_entityManager->UnregisterComponent<Graphics::Components::ParticleEmitter>());
	PE_CHECK(result, m_entityManager->UnregisterComponent<Graphics::Components::MeshRenderer>());
	PE_CHECK(result, m_entityManager->UnregisterComponent<Scene::Components::Transform>());
	PE_CHECK(result, m_entityManager->UnregisterComponent<Scene::Components::DayNightCycle>());
	PE_CHECK(result, m_entityManager->UnregisterComponent<Scene::Components::Tag>());
	return result;
}

ERROR_CODE Engine::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;

	m_state = SystemState::ShuttingDown;
	Utilities::SafeShutdown(m_dayNightSystem);
	Utilities::SafeShutdown(m_sceneLoader);
	Utilities::SafeShutdown(m_sceneControlSystem);
	Utilities::SafeShutdown(m_renderSystem);
	Utilities::SafeShutdown(m_particleSystem);
	Utilities::SafeShutdown(m_guiSystem);
	Utilities::SafeShutdown(m_cameraSystem);
	Utilities::SafeShutdown(m_transformSystem);
	ShutdownComponents();
	Utilities::SafeShutdown(m_entityManager);
	Assets::AssetManager::Shutdown();
	m_state = SystemState::Uninitialized;

	return ERROR_CODE::OK;
}
}  // namespace PE::Core