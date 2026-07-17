#pragma once

#include "ECS/ECSManager.h"
#include "Graphics/RenderConfig.h"
#include "Input/InputSystem.h"
#include "Scene/Components/Transform.h"

namespace PE::Graphics::Systems {
enum class CameraIndex : uint8_t { One = 0, Two = 1, Three = 2, Four = 3 };

class CameraSystem : public ECS::ISystem {
public:
	CameraSystem()			 = default;
	~CameraSystem() override = default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::ECSManager *ecsManager, Input::InputSystem *inputSystem);
	ERROR_CODE Shutdown() override;
	void	   OnUpdate(float dt) override;

	void						SelectActiveCamera(ECS::EntityID activeCamID);
	[[nodiscard]] ECS::EntityID GetActiveCameraEntityID() const { return m_activeCamera; }
	void						MarkDirty(ECS::EntityID entityID) const;
	void						OnResize(float aspectRatio) const;
	[[nodiscard]] Math::Mat44	UpdateViewMatrix(const Scene::Components::Transform &transform) const;

private:
	ECS::ECSManager	   *ref_eM			= nullptr;
	Input::InputSystem *ref_inputSystem = nullptr;
	ECS::EntityID		m_activeCamera	= ECS::INVALID_ENTITY_ID;
};
}  // namespace PE::Graphics::Systems