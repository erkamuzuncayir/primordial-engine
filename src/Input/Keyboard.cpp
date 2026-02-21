#include "Input/Keyboard.h"

namespace PE::Input {
bool Keyboard::IsDown(const int key) const {
	if (key < 0 || key >= 512) return false;
	return m_keys[key];
}

bool Keyboard::IsPressed(const int key) const {
	if (key < 0 || key >= 512) return false;
	return m_keys[key] && !m_prevKeys[key];
}

bool Keyboard::IsReleased(const int key) const {
	if (key < 0 || key >= 512) return false;
	return !m_keys[key] && m_prevKeys[key];
}

void Keyboard::Update() { m_prevKeys = m_keys; }

void Keyboard::SetKeyState(const int key, const bool state) {
	if (key >= 0 && key < 512) {
		m_keys[key] = state;
	}
}
}  // namespace PE::Input