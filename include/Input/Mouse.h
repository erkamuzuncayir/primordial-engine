#pragma once
#include <array>

namespace PE::Input {
class Mouse {
public:
	[[nodiscard]] bool IsButtonDown(int button) const;
	[[nodiscard]] bool IsButtonPressed(int button) const;
	[[nodiscard]] bool IsButtonReleased(int button) const;

	[[nodiscard]] double GetX() const;
	[[nodiscard]] double GetY() const;
	[[nodiscard]] double GetDeltaX() const;
	[[nodiscard]] double GetDeltaY() const;
	[[nodiscard]] double GetScroll() const;

	void Update();

	void SetButtonState(int button, bool state);
	void SetPosition(double x, double y);
	void SetScroll(double offset);

private:
	std::array<bool, 8> m_buttons	  = {false};
	std::array<bool, 8> m_prevButtons = {false};

	double m_x			 = 0.0;
	double m_y			 = 0.0;
	double m_prevX		 = 0.0;
	double m_prevY		 = 0.0;
	double m_scrollDelta = 0.0;
};
}  // namespace PE::Input