#pragma once
#include <string>

#include "Common/Common.h"
#include "EngineConfig.h"
#include "Graphics/Systems/GUISystem.h"
#include "Graphics/Systems/ParticleSystem.h"
#include "Graphics/Systems/RenderSystem.h"
#include "Input/InputSystem.h"
#include "Platform/PlatformSystem.h"
#include "Scene/SceneLoader.h"
#include "Scene/Systems/DayNightSystem.h"
#include "Scene/Systems/SceneControlSystem.h"
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
	ERROR_CODE InitializeComponents(const EngineConfig &config) const;
	void	   UpdateApplication(float dt);
	ERROR_CODE ShutdownComponents() const;
	ERROR_CODE Shutdown();

	void RequestToCloseApplication() const { ref_platformSystem->RequestToCloseTheApplication(); }
	[[nodiscard]] Graphics::Systems::RenderSystem *GetRenderSystem() const { return m_renderSystem; }

private:
	Assets::AssetManager			   *s_assetManager		 = nullptr;
	Platform::PlatformSystem		   *ref_platformSystem	 = nullptr;
	Input::InputSystem				   *ref_inputSystem		 = nullptr;
	SystemState							m_state				 = SystemState::Uninitialized;
	ECS::EntityManager				   *m_entityManager		 = nullptr;
	Graphics::Systems::RenderSystem	   *m_renderSystem		 = nullptr;
	Graphics::Systems::ParticleSystem  *m_particleSystem	 = nullptr;
	Graphics::Systems::CameraSystem	   *m_cameraSystem		 = nullptr;
	Graphics::Systems::GUISystem	   *m_guiSystem			 = nullptr;
	Scene::Systems::TransformSystem	   *m_transformSystem	 = nullptr;
	Scene::Systems::SceneControlSystem *m_sceneControlSystem = nullptr;
	Scene::SceneLoader				   *m_sceneLoader		 = nullptr;
	Scene::Systems::DayNightSystem	   *m_dayNightSystem	 = nullptr;

	std::unordered_map<std::string, ECS::EntityID> m_nameEntityIDMap;
};
}  // namespace PE::Core