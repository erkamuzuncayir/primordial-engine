#pragma once

#include <functional>

#include "Common/Common.h"
#include "InputCode.h"
#include "InputTypes.h"
#include "Keyboard.h"
#include "Mouse.h"

namespace PE::Input {
class InputSystem {
public:
	InputSystem()								= default;
	InputSystem(const InputSystem &)			= delete;
	InputSystem &operator=(const InputSystem &) = delete;
	InputSystem(InputSystem &&)					= delete;
	InputSystem &operator=(InputSystem &&)		= delete;
	~InputSystem()								= default;

	ERROR_CODE Initialize();
	ERROR_CODE Shutdown();
	void	   OnUpdate(float dt);

	void HandleKey(int rawKey, int action, int mods);
	void HandleMouseButton(int button, int action, int mods);
	void HandleMouseMove(double xPos, double yPos);
	void HandleScroll(double xOffset, double yOffset);

	[[nodiscard]] const Keyboard &GetKeyboard() const { return m_keyboard; }
	[[nodiscard]] const Mouse	 &GetMouse() const { return m_mouse; }

	void BindKey(ActionID action, KeyBinding binding);
	void UnbindKey(KeyBinding binding);
	void BindMouseButton(ActionID action, MouseBinding binding);
	void UnbindMouseButton(MouseBinding binding);

	CallbackID Subscribe(const InputAction &inputAction);
	void	   Unsubscribe(const InputAction &inputAction);

private:
	struct Listener {
		CallbackID	  ID;
		InputCallback Callback;
	};

	void Dispatch(ActionID action, ActionType type, float value, InputState state);

	std::unordered_map<int, ActionID> m_keyInProcess;

	std::unordered_map<KeyBinding, ActionID, KeyBindingHasher>	   m_keyBindings;
	std::unordered_map<MouseBinding, ActionID, MouseBindingHasher> m_mouseBindings;

	std::unordered_map<ActionID, std::vector<Listener>> m_startListeners;
	std::unordered_map<ActionID, std::vector<Listener>> m_continueListeners;
	std::unordered_map<ActionID, std::vector<Listener>> m_endListeners;

	std::unordered_map<ActionID, int> m_activeActions;

	SystemState m_state = SystemState::Uninitialized;
	Keyboard	m_keyboard;
	Mouse		m_mouse;
	bool		m_isInitialized	 = false;
	uint32_t	m_nextCallbackID = 0;
};
}  // namespace PE::Input