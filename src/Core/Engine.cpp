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
	m_ecsManager = new ECS::ECSManager();

	// Initialize Internal
	PE_ENSURE_INIT_SILENT(result, m_ecsManager->Initialize(config));
	PE_ENSURE_INIT_SILENT(result, InitializeComponents(config));
	PE_ENSURE_INIT_SILENT(result, InitializeSystems(config, window));

	// Initialize Assets
	PE_ENSURE_INIT_SILENT(result, Assets::AssetManager::Initialize(m_renderSystem.GetRenderer(), config));
	PE_ENSURE_INIT_SILENT(result, m_renderSystem.GetRenderer()->CreateDefaultResources());

	// Initialize Demo Scene
	PE_ENSURE_INIT_SILENT(result,
						  m_sceneLoader.Initialize(m_ecsManager, config.renderConfig, m_renderSystem.GetRenderer()));
	const std::filesystem::path demoScenePath =
		std::filesystem::path("demo-scenes") / "desert-globe" / "desert-globe.ini";
	m_sceneLoader.LoadScene(Utilities::IOUtilities::GetAssetPath(demoScenePath.string()));
	m_sceneManager.SelectControlledEntity(Graphics::Systems::CameraType::Overview);

	m_state = SystemState::Running;
	return result;
}

ERROR_CODE Engine::InitializeSystems(EngineConfig &config, GLFWwindow *window) {
	ERROR_CODE result;
	// TODO: Remove and move to game logic
	PE_ENSURE_INIT(result, m_dayNightSystem.Initialize(ECS::ESystemStage::Logic, m_ecsManager),
				   "Scene control system can't initialized.");
	PE_ENSURE_INIT(result,
				   m_transformSystem.Initialize(ECS::ESystemStage::Transform, m_ecsManager, ref_inputSystem,
												&m_cameraSystem, config),
				   "Transform system can't initialized.");
	PE_ENSURE_INIT(
		result,
		m_cameraSystem.Initialize(ECS::ESystemStage::Camera, m_ecsManager, ref_inputSystem, config.renderConfig),
		"Camera system can't initialized.");
	PE_ENSURE_INIT(result,
				   m_renderSystem.Initialize(ECS::ESystemStage::Render, m_ecsManager, &m_cameraSystem, window, config),
				   "Render system can't initialized.");
	PE_ENSURE_INIT(result,
				   m_particleSystem.Initialize(ECS::ESystemStage::Particle, m_ecsManager, m_renderSystem.GetRenderer()),
				   "Render system can't initialized.");
	PE_ENSURE_INIT(result, m_guiSystem.Initialize(ECS::ESystemStage::GUI, m_ecsManager, m_renderSystem.GetRenderer()),
				   "GUI System failed to initialize.");
	PE_ENSURE_INIT(
		result,
		m_sceneManager.Initialize(ECS::ESystemStage::SceneManager, this, m_ecsManager, &m_sceneLoader, ref_inputSystem,
								  &m_transformSystem, &m_cameraSystem, &m_guiSystem, &m_dayNightSystem, config),
		"Scene control system can't initialized.");

	PE_CHECK(result, m_ecsManager->RegisterSystem(&m_dayNightSystem));
	PE_CHECK(result, m_ecsManager->RegisterSystem(&m_sceneManager));
	PE_CHECK(result, m_ecsManager->RegisterSystem(&m_transformSystem));
	PE_CHECK(result, m_ecsManager->RegisterSystem(&m_cameraSystem));
	PE_CHECK(result, m_ecsManager->RegisterSystem(&m_particleSystem));
	PE_CHECK(result, m_ecsManager->RegisterSystem(&m_guiSystem));
	PE_CHECK(result, m_ecsManager->RegisterSystem(&m_renderSystem));

	return result;
}

ERROR_CODE Engine::InitializeComponents(const EngineConfig &config) const {
	ERROR_CODE result;
	PE_CHECK(result, m_ecsManager->RegisterComponent<Scene::Components::Tag>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager->RegisterComponent<Scene::Components::Transform>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager->RegisterComponent<Graphics::Components::MeshRenderer>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager->RegisterComponent<Graphics::Components::Camera>(config.renderConfig.maxCameraCount));
	PE_CHECK(result, m_ecsManager->RegisterComponent<Graphics::Components::DirectionalLight>(
						 config.renderConfig.maxDirectionalLightCount));
	PE_CHECK(result, m_ecsManager->RegisterComponent<Graphics::Components::ParticleEmitter>(config.maxEntityCount));
	PE_CHECK(result, m_ecsManager->RegisterComponent<Scene::Components::DayNightCycle>(1));
	return result;
}

void Engine::UpdateApplication(const float dt) { m_ecsManager->Update(dt); }

ERROR_CODE Engine::ShutdownComponents() const {
	ERROR_CODE result;
	PE_CHECK(result, m_ecsManager->UnregisterComponent<Graphics::Components::DirectionalLight>());
	PE_CHECK(result, m_ecsManager->UnregisterComponent<Graphics::Components::Camera>());
	PE_CHECK(result, m_ecsManager->UnregisterComponent<Graphics::Components::ParticleEmitter>());
	PE_CHECK(result, m_ecsManager->UnregisterComponent<Graphics::Components::MeshRenderer>());
	PE_CHECK(result, m_ecsManager->UnregisterComponent<Scene::Components::Transform>());
	PE_CHECK(result, m_ecsManager->UnregisterComponent<Scene::Components::DayNightCycle>());
	PE_CHECK(result, m_ecsManager->UnregisterComponent<Scene::Components::Tag>());
	return result;
}

ERROR_CODE Engine::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;

	m_state			  = SystemState::ShuttingDown;
	ERROR_CODE result = ERROR_CODE::OK;
	PE_CHECK(result, ShutdownComponents());
	Utilities::SafeShutdown(m_ecsManager);
	Assets::AssetManager::Shutdown();
	m_state = SystemState::Uninitialized;

	return result;
}
}  // namespace PE::Core