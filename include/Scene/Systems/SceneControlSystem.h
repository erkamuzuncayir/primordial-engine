#pragma once
#include "Core/EngineConfig.h"
#include "DayNightSystem.h"
#include "ECS/ISystem.h"
#include "Graphics/Systems/CameraSystem.h"
#include "Scene/SceneLoader.h"
#include "TransformSystem.h"

namespace PE::Graphics::Systems {
class GUISystem;
}

namespace PE::Core {
class Engine;
}

namespace PE::Scene::Systems {
enum class MovementState : uint8_t {
	RotateUp,
	RotateDown,
	RotateLeft,
	RotateRight,
	PanUp,
	PanDown,
	PanLeft,
	PanRight,
	PanForward,
	PanBackward,
	Count
};

class SceneControlSystem : public ECS::ISystem {
public:
	SceneControlSystem()		   = default;
	~SceneControlSystem() override = default;
	ERROR_CODE Initialize(ECS::ESystemStage stage, Core::Engine *application, ECS::EntityManager *entityManager,
						  SceneLoader *sceneLoader, Input::InputSystem *inputSystem, TransformSystem *transformSystem,
						  Graphics::Systems::CameraSystem *cameraSystem, Graphics::Systems::GUISystem *guiSystem,
						  DayNightSystem *dayNightSystem, const Core::EngineConfig &config);
	ERROR_CODE Shutdown() override;
	void	   OnUpdate(float dt) override;

	void SelectControlledEntity(ECS::EntityID id);
	void SelectControlledEntity(Graphics::Systems::CameraType camType);

private:
	void ProcessObjectMovement(float dt);
	void SetupInputBindings();
	void CleanupInputBindings();

	Core::Engine					*ref_application	 = nullptr;
	ECS::EntityManager				*ref_eM				 = nullptr;
	Input::InputSystem				*ref_inputSystem	 = nullptr;
	SceneLoader						*ref_sceneLoader	 = nullptr;
	Graphics::Systems::CameraSystem *ref_cameraSystem	 = nullptr;
	Graphics::Systems::GUISystem	*ref_guiSystem		 = nullptr;
	TransformSystem					*ref_transformSystem = nullptr;
	DayNightSystem					*ref_dayNightSystem	 = nullptr;
	const Core::EngineConfig		*ref_config{};

	// TODO: Put into a config file (e.g. SceneConfig)
	static constexpr float unitChangeOnPosition = 25.0f;
	static constexpr float unitChangeOnRotation = 1.0f;

	std::vector<Input::InputAction> m_subscribedInputActionIDs;
	std::vector<MovementState>		m_moveStateStacks;
	ECS::EntityID					m_controlledEntity = UINT32_MAX;

	// Demo specific values
	float									 m_fireEffectEndTime = 0.0f;
	bool									 m_isFireActive		 = false;
	static inline constexpr std::string_view burningTreeName{"F4_Trigger_Tree"};
	ECS::EntityID							 m_burningTreeID = ECS::INVALID_ENTITY_ID;
};
}  // namespace PE::Scene::Systems