#include "Input/InputSystem.h"

#include "Platform/PlatformSystem.h"
#include "Utilities/Logger.h"

namespace PE::Input {
ERROR_CODE InputSystem::Initialize() {
	PE_CHECK_STATE_INIT(m_state, "Render system is already initialized!");
	m_state = SystemState::Initializing;

	m_keyBindings.clear();
	m_mouseBindings.clear();
	m_startListeners.clear();
	m_continueListeners.clear();
	m_endListeners.clear();
	m_activeActions.clear();
	m_state = SystemState::Running;

	return ERROR_CODE::OK;
}

ERROR_CODE InputSystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	m_keyBindings.clear();
	m_mouseBindings.clear();
	m_startListeners.clear();
	m_continueListeners.clear();
	m_endListeners.clear();
	m_activeActions.clear();
	m_state = SystemState::Uninitialized;

	return ERROR_CODE::OK;
}

void InputSystem::OnUpdate(const float dt) {
	m_keyboard.Update();
	m_mouse.Update();

	for (const auto &[actionId, refCount] : m_activeActions) {
		if (refCount > 0) {
			Dispatch(actionId, ActionType::Continue, dt, InputState::Repeat);
		}
	}
}

void InputSystem::Dispatch(ActionID action, ActionType type, float value, InputState state) {
	auto &map = (type == ActionType::Start) ? m_startListeners
											: (type == ActionType::Continue ? m_continueListeners : m_endListeners);

	if (const auto it = map.find(action); it != map.end()) {
		const InputContext ctx{action, value, state};
		for (const auto &listener : it->second) {
			listener.Callback(ctx);
		}
	}
}

void InputSystem::HandleKey(const int rawKey, const int action, const int mods) {
	if (rawKey < 0) return;

	const bool isPressed = (action != static_cast<int>(InputState::Release));
	m_keyboard.SetKeyState(rawKey, isPressed);

	if (action == static_cast<int>(InputState::Press)) {
		const KeyBinding bind{static_cast<KeyCode>(rawKey), static_cast<KeyModifier>(mods)};

		if (const auto it = m_keyBindings.find(bind); it != m_keyBindings.end()) {
			const ActionID actId = it->second;

			m_keyInProcess[rawKey] = actId;

			m_activeActions[actId]++;
			Dispatch(actId, ActionType::Start, 1.0f, InputState::Press);
		}
	} else if (action == static_cast<int>(InputState::Release)) {
		if (const auto it = m_keyInProcess.find(rawKey); it != m_keyInProcess.end()) {
			const ActionID actId = it->second;

			if (m_activeActions[actId] > 0) m_activeActions[actId]--;
			Dispatch(actId, ActionType::End, 0.0f, InputState::Release);

			m_keyInProcess.erase(it);
		}
	}
}

void InputSystem::HandleMouseButton(int button, const int action, const int mods) {
	const bool isPressed = (action != static_cast<int>(InputState::Release));
	m_mouse.SetButtonState(button, isPressed);

	const MouseBinding bind{static_cast<MouseCode>(button), static_cast<KeyModifier>(mods)};

	if (const auto it = m_mouseBindings.find(bind); it != m_mouseBindings.end()) {
		const ActionID actId = it->second;

		if (action == static_cast<int>(InputState::Press)) {
			m_activeActions[actId]++;
			Dispatch(actId, ActionType::Start, 1.0f, InputState::Press);
		} else if (action == static_cast<int>(InputState::Release)) {
			if (m_activeActions[actId] > 0) m_activeActions[actId]--;
			Dispatch(actId, ActionType::End, 0.0f, InputState::Release);
		}
	}
}

void InputSystem::HandleMouseMove(double xPos, double yPos) { m_mouse.SetPosition(xPos, yPos); }

void InputSystem::HandleScroll(double xOffset, double yOffset) { m_mouse.SetScroll(yOffset); }

void InputSystem::BindKey(const ActionID action, const KeyBinding binding) { m_keyBindings[binding] = action; }

void InputSystem::UnbindKey(const KeyBinding binding) {
	if (const auto it = m_keyBindings.find(binding); it != m_keyBindings.end()) {
		m_keyBindings.erase(it);
	}
}

void InputSystem::BindMouseButton(const ActionID action, const MouseBinding binding) {
	m_mouseBindings[binding] = action;
}

void InputSystem::UnbindMouseButton(const MouseBinding binding) {
	if (const auto it = m_mouseBindings.find(binding); it != m_mouseBindings.end()) {
		m_mouseBindings.erase(it);
	}
}

CallbackID InputSystem::Subscribe(const InputAction &inputAction) {
	auto &map = (inputAction.type == ActionType::Start)
					? m_startListeners
					: (inputAction.type == ActionType::Continue ? m_continueListeners : m_endListeners);

	map[inputAction.actionID].push_back(Listener{++m_nextCallbackID, std::move(inputAction.callback)});
	return m_nextCallbackID;
}

void InputSystem::Unsubscribe(const InputAction &inputAction) {
	auto &map = (inputAction.type == ActionType::Start)
					? m_startListeners
					: (inputAction.type == ActionType::Continue ? m_continueListeners : m_endListeners);

	if (const auto it = map.find(inputAction.actionID); it != map.end()) {
		auto &listeners = it->second;
		for (auto listIt = listeners.begin(); listIt != listeners.end(); ++listIt) {
			if (listIt->ID == inputAction.callbackID) {
				listeners.erase(listIt);
				return;
			}
		}
	}
}
}  // namespace PE::Input