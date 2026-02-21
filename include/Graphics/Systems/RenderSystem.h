#pragma once
#include "../../ECS/ISystem.h"
#include "CameraSystem.h"
#include "ECS/EntityManager.h"
#include "Graphics/IRenderer.h"
#include "Graphics/RenderConfig.h"
#include "Graphics/Vulkan/VulkanRenderer.h"

namespace PE::Graphics::Systems {
class RenderSystem : public ECS::ISystem {
public:
	RenderSystem()								  = default;
	RenderSystem(const RenderSystem &)			  = delete;
	RenderSystem &operator=(const RenderSystem &) = delete;
	RenderSystem(RenderSystem &&)				  = delete;
	RenderSystem &operator=(RenderSystem &&)	  = delete;
	~RenderSystem() override					  = default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::EntityManager *entityManager, CameraSystem *cameraSystem,
						  GLFWwindow *window, Core::EngineConfig &config);
	ERROR_CODE Shutdown() override;

	void OnUpdate(float dt) override;
	void OnResize(const RenderConfig &config);

	template <typename T>
	T *GetRenderer() const {
		return static_cast<T *>(m_renderer);
	}

	[[nodiscard]] IRenderer *GetRenderer() const { return m_renderer; }

private:
	ERROR_CODE InitializeRenderer(GLFWwindow *window, const Core::EngineConfig &config);

	ECS::EntityManager *ref_entityManager = nullptr;
	CameraSystem	   *ref_cameraSystem  = nullptr;
	Core::EngineConfig *ref_engineConfig;
	RenderConfig	   *ref_renderConfig;
	ECS::EntityID		ref_activeCamEntityID = UINT32_MAX;
	RenderPathType		m_currentPathType;
	IRenderer		   *m_renderer = nullptr;
};
}  // namespace PE::Graphics::Systems