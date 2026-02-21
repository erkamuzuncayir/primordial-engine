#pragma once
#include "GLFW/glfw3.h"

namespace PE::Platform {
/**
 * @brief The global class containing all static GLFW callbacks.
 * These functions handle events via the CallbackManager.
 * Other callbacks can be added here if required (scroll, char, etc.)
 */
class GLFWCallbacks {
public:
	GLFWCallbacks() = delete;

	static void StaticKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void StaticCursorPosCallback(GLFWwindow *window, double xPos, double yPos);
	static void StaticMouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
	static void StaticFramebufferSizeCallback(GLFWwindow *window, int width, int height);
	static void StaticWindowFocusCallback(GLFWwindow *window, int focused);
	static void StaticWindowCloseCallback(GLFWwindow *window);
};
}  // namespace PE::Platform