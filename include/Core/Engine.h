#pragma once
#include <string>

#include "Common/Common.h"
#include "EngineConfig.h"
#include "Graphics/Systems/GUISystem.h"
#include "Graphics/Systems/ParticleSystem.h"
#include "Graphics/Systems/RenderSystem.h"
#include "Input/InputSystem.h"
#include "Physics/Body/Systems/AABBUpdateSystem.h"
#include "Physics/Core/Systems/PhysicsSystem.h"
#include "Platform/PlatformSystem.h"
#include "Scene/SceneLoader.h"
#include "Scene/Systems/DayNightSystem.h"
#include "Scene/Systems/SceneManager.h"
#include "Scene/Systems/TransformSystem.h"

namespace PE::Core {
class Engine {
public:
	Engine()						   = default;
	Engine(const Engine &)			   = delete;
	Engine &operator=(const Engine &)  = delete;
	Engine(const Engine &&)			   = delete;
	Engine &operator=(const Engine &&) = delete;
	~Engine()						   = default;

	ERROR_CODE Initialize(Platform::PlatformSystem *platformSystem, EngineConfig &config, GLFWwindow *window,
						  Input::InputSystem *inputSystem);
	ERROR_CODE InitializeSystems(EngineConfig &config, GLFWwindow *window);
	ERROR_CODE InitializeComponents(const EngineConfig &config);
	void	   UpdateApplication(float dt);
	ERROR_CODE ShutdownComponents();
	ERROR_CODE Shutdown();

	void RequestToCloseApplication() const { ref_platformSystem->RequestToCloseTheApplication(); }
	[[nodiscard]] const Graphics::Systems::RenderSystem &GetRenderSystem() const { return m_renderSystem; }

private:
	Assets::AssetManager				 *   s_assetManager     = nullptr;
	Platform::PlatformSystem			 *   ref_platformSystem = nullptr;
	Input::InputSystem					 * ref_inputSystem    = nullptr;
	SystemState                              m_state            = SystemState::Uninitialized;
	ECS::ECSManager						 m_ecsManager;
	Graphics::Systems::RenderSystem          m_renderSystem;
	Graphics::Systems::ParticleSystem        m_particleSystem;
	Graphics::Systems::CameraSystem          m_cameraSystem;
	Graphics::Systems::GUISystem             m_guiSystem;
	Scene::Systems::TransformSystem          m_transformSystem;
	Scene::Systems::SceneManager             m_sceneManager;
	Scene::SceneLoader                       m_sceneLoader;
	Scene::Systems::DayNightSystem           m_dayNightSystem;
	Physics::Core::Systems::PhysicsSystem    m_physicsSystem;
	// TODO: Move this into physics core?
	Physics::Body::Systems::AABBUpdateSystem	   m_aabbUpdateSystem;
	std::unordered_map<std::string, ECS::EntityID> m_nameEntityIDMap;
};
}  // namespace PE::Core