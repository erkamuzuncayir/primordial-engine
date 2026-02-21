#include "Input/Mouse.h"

namespace PE::Input {
bool Mouse::IsButtonDown(const int button) const {
	if (button < 0 || button >= 8) return false;
	return m_buttons[button];
}

bool Mouse::IsButtonPressed(const int button) const {
	if (button < 0 || button >= 8) return false;
	return m_buttons[button] && !m_prevButtons[button];
}

bool Mouse::IsButtonReleased(const int button) const {
	if (button < 0 || button >= 8) return false;
	return !m_buttons[button] && m_prevButtons[button];
}

double Mouse::GetX() const { return m_x; }
double Mouse::GetY() const { return m_y; }
double Mouse::GetDeltaX() const { return m_x - m_prevX; }
double Mouse::GetDeltaY() const { return m_y - m_prevY; }
double Mouse::GetScroll() const { return m_scrollDelta; }

void Mouse::Update() {
	m_prevButtons = m_buttons;
	m_prevX		  = m_x;
	m_prevY		  = m_y;
	m_scrollDelta = 0.0;
}

void Mouse::SetButtonState(const int button, const bool state) {
	if (button >= 0 && button < 8) {
		m_buttons[button] = state;
	}
}

void Mouse::SetPosition(const double x, const double y) {
	m_x = x;
	m_y = y;
}

void Mouse::SetScroll(const double offset) { m_scrollDelta = offset; }
}  // namespace PE::Input