#pragma once
#include <cstdint>

namespace PE::Input {
// -------------------------------------------------------------------------
// All codes are based on GLFW 3.4.0.
// -------------------------------------------------------------------------

// -------------------------------------------------------------------------
// Input States
// -------------------------------------------------------------------------
enum class InputState : uint8_t { Release = 0, Press = 1, Repeat = 2 };

// -------------------------------------------------------------------------
// Joystick Hat States
// -------------------------------------------------------------------------
enum class HatState : uint8_t {
	Centered  = 0,
	Up		  = 1,
	Right	  = 2,
	Down	  = 4,
	Left	  = 8,
	RightUp	  = (Right | Up),
	RightDown = (Right | Down),
	LeftUp	  = (Left | Up),
	LeftDown  = (Left | Down)
};

// -------------------------------------------------------------------------
// Keyboard Keys
// -------------------------------------------------------------------------
enum class KeyCode : uint16_t  // int16_t to handle 'Unknown = UINT16_MAX'
{
	Unknown = UINT16_MAX,

	// Printable Keys
	Space		 = 32,
	Apostrophe	 = 39, /* ' */
	Comma		 = 44, /* , */
	Minus		 = 45, /* - */
	Period		 = 46, /* . */
	Slash		 = 47, /* / */
	D0			 = 48,
	D1			 = 49,
	D2			 = 50,
	D3			 = 51,
	D4			 = 52,
	D5			 = 53,
	D6			 = 54,
	D7			 = 55,
	D8			 = 56,
	D9			 = 57,
	Semicolon	 = 59, /* ; */
	Equal		 = 61, /* = */
	A			 = 65,
	B			 = 66,
	C			 = 67,
	D			 = 68,
	E			 = 69,
	F			 = 70,
	G			 = 71,
	H			 = 72,
	I			 = 73,
	J			 = 74,
	K			 = 75,
	L			 = 76,
	M			 = 77,
	N			 = 78,
	O			 = 79,
	P			 = 80,
	Q			 = 81,
	R			 = 82,
	S			 = 83,
	T			 = 84,
	U			 = 85,
	V			 = 86,
	W			 = 87,
	X			 = 88,
	Y			 = 89,
	Z			 = 90,
	LeftBracket	 = 91,	/* [ */
	Backslash	 = 92,	/* \ */
	RightBracket = 93,	/* ] */
	GraveAccent	 = 96,	/* ` */
	World1		 = 161, /* non-US #1 */
	World2		 = 162, /* non-US #2 */

	// Function Keys
	Escape		 = 256,
	Enter		 = 257,
	Tab			 = 258,
	Backspace	 = 259,
	Insert		 = 260,
	Delete		 = 261,
	Right		 = 262,
	Left		 = 263,
	Down		 = 264,
	Up			 = 265,
	PageUp		 = 266,
	PageDown	 = 267,
	Home		 = 268,
	End			 = 269,
	CapsLock	 = 280,
	ScrollLock	 = 281,
	NumLock		 = 282,
	PrintScreen	 = 283,
	Pause		 = 284,
	F1			 = 290,
	F2			 = 291,
	F3			 = 292,
	F4			 = 293,
	F5			 = 294,
	F6			 = 295,
	F7			 = 296,
	F8			 = 297,
	F9			 = 298,
	F10			 = 299,
	F11			 = 300,
	F12			 = 301,
	F13			 = 302,
	F14			 = 303,
	F15			 = 304,
	F16			 = 305,
	F17			 = 306,
	F18			 = 307,
	F19			 = 308,
	F20			 = 309,
	F21			 = 310,
	F22			 = 311,
	F23			 = 312,
	F24			 = 313,
	F25			 = 314,
	KP0			 = 320,
	KP1			 = 321,
	KP2			 = 322,
	KP3			 = 323,
	KP4			 = 324,
	KP5			 = 325,
	KP6			 = 326,
	KP7			 = 327,
	KP8			 = 328,
	KP9			 = 329,
	KPDecimal	 = 330,
	KPDivide	 = 331,
	KPMultiply	 = 332,
	KPSubtract	 = 333,
	KPAdd		 = 334,
	KPEnter		 = 335,
	KPEqual		 = 336,
	LeftShift	 = 340,
	LeftControl	 = 341,
	LeftAlt		 = 342,
	LeftSuper	 = 343,
	RightShift	 = 344,
	RightControl = 345,
	RightAlt	 = 346,
	RightSuper	 = 347,
	Menu		 = 348
};

// -------------------------------------------------------------------------
// Modifier Flags (Bitmask)
// -------------------------------------------------------------------------
enum class KeyModifier : uint8_t {
	None	 = 0,
	Shift	 = 0x0001,
	Control	 = 0x0002,
	Alt		 = 0x0004,
	Super	 = 0x0008,
	CapsLock = 0x0010,
	NumLock	 = 0x0020
};

// Helper to allow bitwise OR on KeyModifier class
inline KeyModifier operator|(KeyModifier a, KeyModifier b) {
	return static_cast<KeyModifier>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline bool operator&(KeyModifier a, KeyModifier b) { return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0; }

// -------------------------------------------------------------------------
// Mouse Buttons
// -------------------------------------------------------------------------
enum class MouseCode : uint8_t {
	Button1 = 0,
	Button2 = 1,
	Button3 = 2,
	Button4 = 3,
	Button5 = 4,
	Button6 = 5,
	Button7 = 6,
	Button8 = 7,

	// Aliases
	Left   = Button1,
	Right  = Button2,
	Middle = Button3,
	Last   = Button8
};

// -------------------------------------------------------------------------
// Joystick IDs
// -------------------------------------------------------------------------
enum class JoystickID : uint8_t {
	Joy1  = 0,
	Joy2  = 1,
	Joy3  = 2,
	Joy4  = 3,
	Joy5  = 4,
	Joy6  = 5,
	Joy7  = 6,
	Joy8  = 7,
	Joy9  = 8,
	Joy10 = 9,
	Joy11 = 10,
	Joy12 = 11,
	Joy13 = 12,
	Joy14 = 13,
	Joy15 = 14,
	Joy16 = 15
};

// -------------------------------------------------------------------------
// Gamepad Buttons
// -------------------------------------------------------------------------
enum class GamepadButton : uint8_t {
	A			= 0,
	B			= 1,
	X			= 2,
	Y			= 3,
	LeftBumper	= 4,
	RightBumper = 5,
	Back		= 6,
	Start		= 7,
	Guide		= 8,
	LeftThumb	= 9,
	RightThumb	= 10,
	DPadUp		= 11,
	DPadRight	= 12,
	DPadDown	= 13,
	DPadLeft	= 14,

	// Aliases for PlayStation Controllers
	Cross	 = A,
	Circle	 = B,
	Square	 = X,
	Triangle = Y
};

// -------------------------------------------------------------------------
// Gamepad Axes
// -------------------------------------------------------------------------
enum class GamepadAxis : uint8_t { LeftX = 0, LeftY = 1, RightX = 2, RightY = 3, LeftTrigger = 4, RightTrigger = 5 };
}  // namespace PE::Input