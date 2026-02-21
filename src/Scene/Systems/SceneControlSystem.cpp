#include "Scene/Systems/SceneControlSystem.h"

#include "Core/Engine.h"
#include "Core/EngineConfig.h"
#include "Graphics/Components/Camera.h"
#include "Scene/Components/Tag.h"
#include "Scene/Systems/TransformSystem.h"

namespace PE::Scene::Systems {
struct BindingConfig {
	Input::ActionID	  actionId;
	Input::KeyBinding binding;
};

struct MovementBindingConfig : BindingConfig {
	MovementState targetState;
};

static constexpr BindingConfig SIMULATION_CONFIGS[] = {
	{Input::INPUT_ACTION("Exit"), {Input::KeyCode::Escape, Input::KeyModifier::None}},
	{Input::INPUT_ACTION("Reset"), {Input::KeyCode::R, Input::KeyModifier::None}},
	{Input::INPUT_ACTION("Burn"), {Input::KeyCode::F4, Input::KeyModifier::None}},
	{Input::INPUT_ACTION("TimeUp"), {Input::KeyCode::T, Input::KeyModifier::Shift}},
	{Input::INPUT_ACTION("TimeDown"), {Input::KeyCode::T, Input::KeyModifier::None}},
	{Input::INPUT_ACTION("ToggleGUI"), {Input::KeyCode::U, Input::KeyModifier::None}},
};

static constexpr MovementBindingConfig MOVEMENT_CONFIGS[] = {
	{Input::INPUT_ACTION("RotateUp"), {Input::KeyCode::W, Input::KeyModifier::None}, MovementState::RotateUp},
	{Input::INPUT_ACTION("RotateDown"), {Input::KeyCode::S, Input::KeyModifier::None}, MovementState::RotateDown},
	{Input::INPUT_ACTION("RotateLeft"), {Input::KeyCode::A, Input::KeyModifier::None}, MovementState::RotateLeft},
	{Input::INPUT_ACTION("RotateRight"), {Input::KeyCode::D, Input::KeyModifier::None}, MovementState::RotateRight},

	{Input::INPUT_ACTION("PanUp"), {Input::KeyCode::W, Input::KeyModifier::Control}, MovementState::PanUp},
	{Input::INPUT_ACTION("PanDown"), {Input::KeyCode::S, Input::KeyModifier::Control}, MovementState::PanDown},
	{Input::INPUT_ACTION("PanLeft"), {Input::KeyCode::A, Input::KeyModifier::Control}, MovementState::PanLeft},
	{Input::INPUT_ACTION("PanRight"), {Input::KeyCode::D, Input::KeyModifier::Control}, MovementState::PanRight},
	{Input::INPUT_ACTION("PanForward"), {Input::KeyCode::Q, Input::KeyModifier::Control}, MovementState::PanForward},
	{Input::INPUT_ACTION("PanBackward"), {Input::KeyCode::E, Input::KeyModifier::Control}, MovementState::PanBackward},
};

struct SelectionBindingConfig : BindingConfig {
	Graphics::Systems::CameraType targetType;
};

static constexpr SelectionBindingConfig SELECTION_CONFIGS[] = {
	{Input::INPUT_ACTION("SelOverview"),
	 {Input::KeyCode::F1, Input::KeyModifier::None},
	 Graphics::Systems::CameraType::Overview},
	{Input::INPUT_ACTION("SelNav"),
	 {Input::KeyCode::F2, Input::KeyModifier::None},
	 Graphics::Systems::CameraType::Navigation},
	{Input::INPUT_ACTION("SelCloseUp"),
	 {Input::KeyCode::F3, Input::KeyModifier::None},
	 Graphics::Systems::CameraType::CloseUp},
};

ERROR_CODE SceneControlSystem::Initialize(ECS::ESystemStage stage, Core::Engine *application,
										  ECS::EntityManager *entityManager, SceneLoader *sceneLoader,
										  Input::InputSystem *inputSystem, TransformSystem *transformSystem,
										  Graphics::Systems::CameraSystem *cameraSystem,
										  Graphics::Systems::GUISystem *guiSystem, DayNightSystem *dayNightSystem,
										  const Core::EngineConfig &config) {
	PE_CHECK_STATE_INIT(m_state, "SceneControl system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID			= GetUniqueISystemTypeID<SceneControlSystem>();
	ref_application		= application;
	ref_eM				= entityManager;
	ref_inputSystem		= inputSystem;
	ref_sceneLoader		= sceneLoader;
	ref_transformSystem = transformSystem;
	ref_cameraSystem	= cameraSystem;
	ref_guiSystem		= guiSystem;
	ref_dayNightSystem	= dayNightSystem;
	ref_config			= &config;
	m_stage				= stage;

	ERROR_CODE result;
	PE_CHECK(result, ref_eM->RegisterSystem(this));

	m_subscribedInputActionIDs.clear();
	m_moveStateStacks.reserve(static_cast<size_t>(MovementState::Count));

	SetupInputBindings();

	m_state = SystemState::Running;
	return result;
}

ERROR_CODE SceneControlSystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	ERROR_CODE result;
	PE_CHECK(result, ref_eM->UnregisterSystem(this));
	m_stage	 = ECS::ESystemStage::Count;
	m_typeID = UINT32_MAX;

	CleanupInputBindings();

	m_moveStateStacks.clear();
	m_subscribedInputActionIDs.clear();

	m_state = SystemState::Uninitialized;
	return result;
}

void SceneControlSystem::OnUpdate(float dt) {
	if ((m_controlledEntity != ECS::INVALID_ENTITY_ID) && !m_moveStateStacks.empty()) ProcessObjectMovement(dt);

	if (m_isFireActive) {
		m_fireEffectEndTime -= dt;

		if (m_fireEffectEndTime <= 0.0f) {
			auto *emitter = ref_eM->GetTIComponent<Graphics::Components::ParticleEmitter>(m_burningTreeID);
			if (emitter) {
				emitter->spawnRate = 0.0f;
			}
			m_isFireActive	= false;
			m_burningTreeID = ECS::INVALID_ENTITY_ID;

			PE_LOG_INFO("Fire effect finished.");
		}
	}
}

void SceneControlSystem::SelectControlledEntity(const ECS::EntityID id) { m_controlledEntity = id; }

// TODO: Refactor this to be better solution based on loaded scene not hardcoded.
void SceneControlSystem::SelectControlledEntity(const Graphics::Systems::CameraType camType) {
	ECS::ComponentArray<Graphics::Components::Camera> &camCompArr = ref_eM->GetCompArr<Graphics::Components::Camera>();
	if (!camCompArr.Has(static_cast<uint8_t>(camType))) return;

	m_controlledEntity = camCompArr.Index()[static_cast<uint8_t>(camType)];
	if (const ECS::EntityID entityID = camCompArr.Index()[static_cast<uint8_t>(camType)];
		entityID != ECS::INVALID_ENTITY_ID) {
		PE_LOG_INFO("Camera type " + std::to_string(static_cast<uint8_t>(camType)) + " is selected!");
		m_controlledEntity = entityID;
		ref_cameraSystem->SelectActiveCamera(entityID);
	}
}

void SceneControlSystem::ProcessObjectMovement(float dt) {
	Math::Vector3 localInput = Math::Vector3Zero;
	Math::Vector3 deltaRot	 = Math::Vector3Zero;

	const size_t size = m_moveStateStacks.size();
	for (uint8_t i = 0; i < size; i++) {
		switch (m_moveStateStacks[i]) {
			case MovementState::RotateUp: deltaRot.x -= unitChangeOnRotation; break;
			case MovementState::RotateDown: deltaRot.x += unitChangeOnRotation; break;
			case MovementState::RotateLeft: deltaRot.y -= unitChangeOnRotation; break;
			case MovementState::RotateRight: deltaRot.y += unitChangeOnRotation; break;
			case MovementState::PanUp: localInput.y += unitChangeOnPosition; break;
			case MovementState::PanDown: localInput.y -= unitChangeOnPosition; break;
			case MovementState::PanLeft: localInput.x -= unitChangeOnPosition; break;
			case MovementState::PanRight: localInput.x += unitChangeOnPosition; break;
			case MovementState::PanForward: localInput.z += unitChangeOnPosition; break;
			case MovementState::PanBackward: localInput.z -= unitChangeOnPosition; break;
			case MovementState::Count: break;
		}
	}

	if (Math::LengthSq(localInput) > 0) {
		Math::Vector3 currentRot   = ref_transformSystem->GetRotation(m_controlledEntity);
		auto [Right, Up, Forward]  = Math::CalculateDirectionVectors(currentRot);
		Math::Vector3 worldMoveDir = (Forward * localInput.z) + (Right * localInput.x) + (Up * localInput.y);
		ref_transformSystem->SetPosition(m_controlledEntity,
										 ref_transformSystem->GetPosition(m_controlledEntity) += (worldMoveDir * dt));
	}
	if (Math::LengthSq(deltaRot) > 0) {
		Math::Vector3 currentRot = ref_transformSystem->GetRotation(m_controlledEntity);

		Math::Vector3 newRot = currentRot + (deltaRot * dt);

		newRot.x = std::clamp(newRot.x, -1.5f, 1.5f);

		ref_transformSystem->SetRotation(m_controlledEntity, newRot);
	}
}

void SceneControlSystem::SetupInputBindings() {
	for (const auto &cfg : MOVEMENT_CONFIGS) {
		ref_inputSystem->BindKey(cfg.actionId, cfg.binding);

		Input::InputAction startAction = {
			.actionID	= cfg.actionId,
			.type		= Input::ActionType::Start,
			.callback	= [this, cfg](const Input::InputContext &) { m_moveStateStacks.push_back(cfg.targetState); },
			.callbackID = UINT32_MAX};

		startAction.callbackID = ref_inputSystem->Subscribe(startAction);
		m_subscribedInputActionIDs.push_back(startAction);

		Input::InputAction endAction = {
			.actionID	= cfg.actionId,
			.type		= Input::ActionType::End,
			.callback	= [this, cfg](const Input::InputContext &) { std::erase(m_moveStateStacks, cfg.targetState); },
			.callbackID = UINT32_MAX};

		endAction.callbackID = ref_inputSystem->Subscribe(endAction);
		m_subscribedInputActionIDs.push_back(endAction);
	}

	for (const auto &cfg : SELECTION_CONFIGS) {
		ref_inputSystem->BindKey(cfg.actionId, cfg.binding);

		Input::InputAction action = {
			.actionID	= cfg.actionId,
			.type		= Input::ActionType::Start,
			.callback	= [this, cfg](const Input::InputContext &) { SelectControlledEntity(cfg.targetType); },
			.callbackID = UINT32_MAX};

		action.callbackID = ref_inputSystem->Subscribe(action);
		m_subscribedInputActionIDs.push_back(action);
	}

	for (const auto &[actionId, binding] : SIMULATION_CONFIGS) {
		ref_inputSystem->BindKey(actionId, binding);

		Input::InputAction action = {.actionID = actionId, .type = Input::ActionType::Start, .callbackID = UINT32_MAX};
		switch (binding.key) {
			case Input::KeyCode::Escape: {
				action.callback = [this](const Input::InputContext &) { ref_application->RequestToCloseApplication(); };
				break;
			}
			case Input::KeyCode::R: {
				action.callback = [this](const Input::InputContext &) { ref_sceneLoader->ReloadScene(); };
				break;
			}
			case Input::KeyCode::U: {
				action.callback = [this](const Input::InputContext &) { ref_guiSystem->ToggleGUI(); };
				break;
			}
			case Input::KeyCode::T: {
				action.callback = [this, binding](const Input::InputContext &) {
					if (binding.mods & Input::KeyModifier::Shift)
						ref_dayNightSystem->DecreaseCycleDayDuration();
					else
						ref_dayNightSystem->IncreaseCycleDayDuration();
				};
				break;
			}
			case Input::KeyCode::F4: {
				action.callback = [this](const Input::InputContext &) {
					auto &compArr = ref_eM->GetCompArr<Components::Tag>();
					for (int i = 0; i < compArr.Data().size(); i++) {
						if (compArr.Data()[i].name == burningTreeName) {
							m_burningTreeID = compArr.Index()[i];
							auto *emitter =
								ref_eM->GetTIComponent<Graphics::Components::ParticleEmitter>(m_burningTreeID);
							if (emitter) {
								emitter->spawnRate = 500.0f;

								m_fireEffectEndTime = 30.0f;
								m_isFireActive		= true;
								SelectControlledEntity(Graphics::Systems::CameraType::CloseUp);
							}
							break;
						}
					}
				};
				break;
			};
			default: PE_LOG_ERROR("Unknown config!"); break;
		}

		action.callbackID = ref_inputSystem->Subscribe(action);
		m_subscribedInputActionIDs.push_back(action);
	}
}

void SceneControlSystem::CleanupInputBindings() {
	for (const auto &ia : m_subscribedInputActionIDs) ref_inputSystem->Unsubscribe(ia);
	for (const auto &config : MOVEMENT_CONFIGS) ref_inputSystem->UnbindKey(config.binding);
	for (const auto &config : SELECTION_CONFIGS) ref_inputSystem->UnbindKey(config.binding);

	m_subscribedInputActionIDs.clear();
}
}  // namespace PE::Scene::Systems