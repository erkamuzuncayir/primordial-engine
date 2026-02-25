#pragma once

#include "ECS/EntityManager.h"
#include "Graphics/RenderConfig.h"
#include "Input/InputSystem.h"
#include "Scene/Components/Transform.h"

namespace PE::Graphics::Systems {
enum class CameraType : uint8_t { Overview = 0, Navigation = 1, CloseUp = 2, Count = 3 };

class CameraSystem : public ECS::ISystem {
public:
	CameraSystem()			 = default;
	~CameraSystem() override = default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::EntityManager *entityManager, Input::InputSystem *inputSystem,
						  const RenderConfig &renderConfig);
	ERROR_CODE Shutdown() override;
	void	   OnUpdate(float dt) override;

	void						SelectActiveCamera(ECS::EntityID activeCamID);
	[[nodiscard]] ECS::EntityID GetActiveCameraEntityID() const { return m_activeCamera; }
	void						MarkDirty(ECS::EntityID entityID) const;
	void						OnResize(float aspectRatio) const;
	[[nodiscard]] Math::Mat44	UpdateViewMatrix(const Scene::Components::Transform &transform) const;

private:
	ECS::EntityManager *ref_eM			= nullptr;
	Input::InputSystem *ref_inputSystem = nullptr;
	ECS::EntityID		m_activeCamera	= ECS::INVALID_ENTITY_ID;
};
}  // namespace PE::Graphics::Systems