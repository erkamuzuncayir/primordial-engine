#pragma once
#include <array>

namespace PE::Input {
class Keyboard {
public:
	[[nodiscard]] bool IsDown(int key) const;
	[[nodiscard]] bool IsPressed(int key) const;
	[[nodiscard]] bool IsReleased(int key) const;

	void Update();
	void SetKeyState(int key, bool state);

private:
	std::array<bool, 512> m_keys	 = {false};
	std::array<bool, 512> m_prevKeys = {false};
};
}  // namespace PE::Input