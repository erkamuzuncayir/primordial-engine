#pragma once
#include "Common/Common.h"
#include "Input/InputSystem.h"

namespace PE::Platform {
class PlatformSystem;

/**
 * @brief Used to bypass GLFW's single 'WindowUserPointer' restriction.
 * This structure is assigned to the window and is used by all static GLFW callbacks
 * to correctly direct events to the appropriate system instance.
 */
struct CallbackManager {
	CallbackManager()									= default;
	CallbackManager(const CallbackManager &)			= delete;
	CallbackManager &operator=(const CallbackManager &) = delete;
	CallbackManager(CallbackManager &&)					= delete;
	CallbackManager &operator=(CallbackManager &&)		= delete;
	~CallbackManager()									= default;

	ERROR_CODE Initialize(PlatformSystem *platformSystem, Input::InputSystem *inputSystem);

	PlatformSystem	   *ref_platformSystem = nullptr;
	Input::InputSystem *ref_inputSystem	   = nullptr;
};
}  // namespace PE::Platform