#pragma once
#include "Common/Common.h"
#include "ECS/Entity.h"
#include "ECS/ISystem.h"
#include "Graphics/IRenderer.h"
#include "RenderSystem.h"
#include "Scene/Systems/SceneManager.h"
#include "Utilities/Timer.h"

namespace PE::Graphics::Systems {
class GUISystem : public ECS::ISystem {
public:
	GUISystem()			  = default;
	~GUISystem() override = default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::ECSManager *ecsManager,
						  Physics::Core::Systems::PhysicsSystem *physicsSystem, Scene::Systems::SceneManager* sceneManager, Scene::SceneLoader *sceneLoader,
						  IRenderer *renderer);
	ERROR_CODE Shutdown() override;
	void	   OnUpdate(float dt) override;
	void	   ToggleGUI();

private:
	void Render() const;

	void DrawPerformanceStats(float dt);
	void DrawTopBar();
	void DrawHierarchy();
	void DrawInspector() const;
	void DrawAssetBrowser();
	void DrawEntityNodeRecursive(uint32_t                                                   entityID,
								 const std::unordered_map<uint32_t, std::vector<uint32_t>> &childrenMap);
	void DrawComponents() const;

	ECS::ECSManager						  *ref_eM			 = nullptr;
	Physics::Core::Systems::PhysicsSystem *ref_physicsSystem = nullptr;
	IRenderer							  *ref_renderer		 = nullptr;
	Scene::SceneLoader					  *ref_sceneLoader	 = nullptr;
	Scene::Systems::SceneManager		*ref_sceneManager = nullptr;

	Utilities::Timer m_fpsTimer;
	bool			  m_shouldRender	  = true;
	bool			  m_shouldPhysicsStop = false;

	ECS::EntityID m_selectedEntity = ECS::INVALID_ENTITY_ID;
};
}  // namespace PE::Graphics::Systems