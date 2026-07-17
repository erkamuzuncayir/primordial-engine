#pragma once
#include "Core/EngineConfig.h"
#include "DayNightSystem.h"
#include "SpawnSystem.h"
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

class SceneManager : public ECS::ISystem {
public:
	SceneManager()			 = default;
	~SceneManager() override = default;
	ERROR_CODE Initialize(ECS::ESystemStage stage, Core::Engine *application, ECS::ECSManager *ecsManager,
						  SceneLoader *sceneLoader, Input::InputSystem *inputSystem, TransformSystem *transformSystem, Physics::Core::Systems::PhysicsSystem* physicsSystem,
						  Graphics::Systems::CameraSystem *cameraSystem, Graphics::Systems::GUISystem *guiSystem,
						  DayNightSystem *dayNightSystem, const Core::EngineConfig &config);
	ERROR_CODE Shutdown() override;


	void OnUpdate(float dt) override;
	void ResetScene();

	void		 ChangeCamera();
	void		 ChangeCamera(uint32_t camIdx);
	SpawnSystem &GetSpawnSystem() { return m_spawnSystem; }
	void		 SelectControlledEntity(ECS::EntityID id);
	void		 SelectControlledEntity(Graphics::Systems::CameraIndex camType);
	void ProcessFireEffect(float dt);

private:
	void ProcessObjectMovement(float dt);
	void SetupInputBindings();
	void CleanupInputBindings();

	Core::Engine					*ref_application	 = nullptr;
	ECS::ECSManager					*ref_eM				 = nullptr;
	Input::InputSystem				*ref_inputSystem	 = nullptr;
	SceneLoader						*ref_sceneLoader	 = nullptr;
	Graphics::Systems::CameraSystem *ref_cameraSystem	 = nullptr;
	Graphics::Systems::GUISystem	*ref_guiSystem		 = nullptr;
	TransformSystem					*ref_transformSystem = nullptr;
	Physics::Core::Systems::PhysicsSystem *ref_physicsSystem = nullptr;
	DayNightSystem					*ref_dayNightSystem	 = nullptr;
	const Core::EngineConfig		*ref_config{};
	SpawnSystem						 m_spawnSystem;

	// TODO: Put into a config file (e.g. SceneConfig)
	static constexpr float unitChangeOnPosition = 25.0f;
	static constexpr float unitChangeOnRotation = 1.0f;

	std::vector<Input::InputAction> m_subscribedInputActionIDs;
	std::vector<MovementState>		m_moveStateStacks;
	ECS::EntityID					m_controlledEntity = ECS::INVALID_ENTITY_ID;

	// Selected entity orientation values
	float m_pitch;
	float m_yaw;

	// Demo specific values
	float									 m_fireEffectEndTime = 0.0f;
	bool									 m_isFireActive		 = false;
	static inline constexpr std::string_view burningTreeName{"F4_Trigger_Tree"};
	ECS::EntityID							 m_burningTreeID = ECS::INVALID_ENTITY_ID;
};
}  // namespace PE::Scene::Systems