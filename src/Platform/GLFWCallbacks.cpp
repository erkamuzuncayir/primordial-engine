#include "Platform/GLFWCallbacks.h"

#include <GLFW/glfw3.h>

#include "Input/InputSystem.h"
#include "Platform/CallbackManager.h"
#include "Platform/PlatformSystem.h"

namespace PE::Platform {
inline CallbackManager *GetManager(GLFWwindow *window) {
	return static_cast<CallbackManager *>(glfwGetWindowUserPointer(window));
}

void GLFWCallbacks::StaticKeyCallback(GLFWwindow *window, const int key, int scancode, const int action,
									  const int mods) {
	if (const auto *mgr = GetManager(window); mgr && mgr->ref_platformSystem)
		mgr->ref_inputSystem->HandleKey(key, action, mods);
}

void GLFWCallbacks::StaticCursorPosCallback(GLFWwindow *window, const double xPos, const double yPos) {
	if (const auto *mgr = GetManager(window); mgr && mgr->ref_inputSystem)
		mgr->ref_inputSystem->HandleMouseMove(xPos, yPos);
}

void GLFWCallbacks::StaticMouseButtonCallback(GLFWwindow *window, const int button, const int action, const int mods) {
	if (const auto *mgr = GetManager(window); mgr && mgr->ref_inputSystem)
		mgr->ref_inputSystem->HandleMouseButton(button, action, mods);
}

void GLFWCallbacks::StaticFramebufferSizeCallback(GLFWwindow *window, const int width, const int height) {
	if (const auto *mgr = GetManager(window); mgr && mgr->ref_platformSystem)
		mgr->ref_platformSystem->OnWindowResize(width, height);
}

void GLFWCallbacks::StaticWindowFocusCallback(GLFWwindow *window, const int focused) {
	if (const auto *mgr = GetManager(window); mgr && mgr->ref_platformSystem)
		mgr->ref_platformSystem->OnWindowFocus(focused);
}

void GLFWCallbacks::StaticWindowCloseCallback(GLFWwindow *window) {
	if (const auto *mgr = GetManager(window); mgr && mgr->ref_platformSystem)
		mgr->ref_platformSystem->RequestToCloseTheApplication();
}
}  // namespace PE::Platform