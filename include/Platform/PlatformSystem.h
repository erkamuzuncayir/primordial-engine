#pragma once
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3.h>

#include "CallbackManager.h"
#include "Core/EngineConfig.h"
#include "Input/InputSystem.h"
#include "Utilities/Timer.h"

namespace PE::Core {
class Engine;
}

namespace PE::Platform {
class PlatformSystem {
public:
	PlatformSystem()								  = default;
	PlatformSystem(const PlatformSystem &)			  = delete;
	PlatformSystem &operator=(const PlatformSystem &) = delete;
	PlatformSystem(PlatformSystem &&)				  = delete;
	PlatformSystem &operator=(PlatformSystem &&)	  = delete;
	~PlatformSystem()								  = default;

	ERROR_CODE Initialize(Core::EngineConfig &config);
	void	   Shutdown();
	void	   Run();
	void	   RequestToCloseTheApplication();

private:
	friend class GLFWCallbacks;

	void CalculateFrameStats() const;
	void OnWindowResize(int width, int height);
	void OnWindowFocus(int focused);
	void OnWindowClose();

	const Core::EngineConfig *ref_config;
	GLFWwindow				 *m_mainWindow		= nullptr;
	CallbackManager			 *m_callbackManager = nullptr;
	Input::InputSystem		 *m_inputSystem		= nullptr;
	Core::Engine			 *m_application		= nullptr;
	Utilities::Timer		  m_timer;

	std::string m_mainWindowCaption = "Primordial Engine";
	bool		m_appShouldClose	= false;
	bool		m_appPaused			= false;
	bool		m_minimized			= false;
	bool		m_maximized			= false;
	bool		m_resizing			= false;
};
}  // namespace PE::Platform