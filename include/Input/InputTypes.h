#pragma once
#include <cstdint>
#include <functional>

namespace PE::Input {
struct InputContext;
using InputCallback = std::function<void(const InputContext &)>;
using CallbackID	= uint32_t;

enum class ActionType { Start, Continue, End };

using ActionID = uint32_t;

constexpr ActionID HashString(const char *str, const size_t n) {
	ActionID hash = 2166136261u;
	for (size_t i = 0; i < n; ++i) {
		hash ^= static_cast<uint8_t>(str[i]);
		hash *= 16777619u;
	}
	return hash;
}

template <size_t N>
constexpr ActionID INPUT_ACTION(const char (&str)[N]) {
	return HashString(str, N - 1);
}

inline ActionID GetActionId(const char *str) {
	size_t len = 0;
	while (str[len] != '\0') len++;
	return HashString(str, len);
}

struct InputAction {
	ActionID	  actionID = UINT32_MAX;
	ActionType	  type;
	InputCallback callback;
	CallbackID	  callbackID = UINT32_MAX;
};

struct InputContext {
	ActionID   Action;
	float	   Value;
	InputState State;
};

struct KeyBinding {
	KeyCode		key;
	KeyModifier mods;

	bool operator==(const KeyBinding &other) const { return key == other.key && mods == other.mods; }
};

struct MouseBinding {
	MouseCode	button;
	KeyModifier mods;

	bool operator==(const MouseBinding &other) const { return button == other.button && mods == other.mods; }
};

struct KeyBindingHasher {
	std::size_t operator()(const KeyBinding &k) const {
		return (static_cast<size_t>(k.key) << 8) | static_cast<size_t>(k.mods);
	}
};

struct MouseBindingHasher {
	std::size_t operator()(const MouseBinding &m) const {
		return (static_cast<size_t>(m.button) << 8) | static_cast<size_t>(m.mods);
	}
};
}  // namespace PE::Input