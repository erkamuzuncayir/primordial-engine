#include "Platform/CallbackManager.h"

namespace PE::Platform {
ERROR_CODE CallbackManager::Initialize(PlatformSystem *platformSystem, Input::InputSystem *inputSystem) {
	ref_platformSystem = platformSystem;
	ref_inputSystem	   = inputSystem;
	return ERROR_CODE::OK;
}
}  // namespace PE::Platform