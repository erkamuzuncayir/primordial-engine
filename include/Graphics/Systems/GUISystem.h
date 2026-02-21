#pragma once
#include "Common/Common.h"
#include "ECS/Entity.h"
#include "ECS/ISystem.h"
#include "Graphics/IRenderer.h"
#include "RenderSystem.h"
#include "Scene/Systems/SceneControlSystem.h"
#include "Utilities/Timer.h"

namespace PE::Graphics::Systems {
class GUISystem : ECS::ISystem {
public:
	GUISystem()			  = default;
	~GUISystem() override = default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::EntityManager *entityManager,
						  Scene::Systems::SceneControlSystem *sceneControlSystem, IRenderer *renderer);
	ERROR_CODE Shutdown() override;
	void	   OnUpdate(float dt) override;
	void	   ToggleGUI();

private:
	void Render() const;

	void DrawPerformanceStats(float dt);
	void DrawHierarchy();
	void DrawInspector();
	void DrawAssetBrowser();

	void DrawEntityNodeRecursive(uint32_t													entityID,
								 const std::unordered_map<uint32_t, std::vector<uint32_t>> &childrenMap);

	ECS::EntityManager *ref_eM		 = nullptr;
	IRenderer		   *ref_renderer = nullptr;

	Utilities::Timer *m_fpsTimer	   = nullptr;
	bool			  m_shouldRender   = true;
	bool			  m_showDemoWindow = false;

	ECS::EntityID m_selectedEntity = ECS::INVALID_ENTITY_ID;
};
}  // namespace PE::Graphics::Systems