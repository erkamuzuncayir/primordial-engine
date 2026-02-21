#include "Platform/PlatformSystem.h"

#include "Core/Engine.h"
#include "Input/InputSystem.h"
#include "Platform/GLFWCallbacks.h"
#include "Utilities/Logger.h"
#include "Utilities/MemoryUtilities.h"

namespace PE::Platform {
ERROR_CODE PlatformSystem::Initialize(Core::EngineConfig &config) {
	if (!glfwInit()) {
		PE_LOG_FATAL("Failed to initialize GLFW.");
		return ERROR_CODE::WINDOW_CREATION_FAILED;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	m_mainWindow = glfwCreateWindow(config.renderConfig.width, config.renderConfig.height, m_mainWindowCaption.c_str(),
									nullptr, nullptr);
	if (!m_mainWindow) {
		glfwTerminate();
		PE_LOG_FATAL("Failed to create GLFW window.");
		return ERROR_CODE::WINDOW_CREATION_FAILED;
	}

	ref_config	  = &config;
	m_inputSystem = new Input::InputSystem;
	ERROR_CODE result;
	PE_ENSURE_INIT_SILENT(result, m_inputSystem->Initialize());

	m_callbackManager = new CallbackManager();
	PE_ENSURE_INIT_SILENT(result, m_callbackManager->Initialize(this, m_inputSystem));

	glfwSetWindowUserPointer(m_mainWindow, m_callbackManager);
	glfwSetKeyCallback(m_mainWindow, GLFWCallbacks::StaticKeyCallback);
	glfwSetCursorPosCallback(m_mainWindow, GLFWCallbacks::StaticCursorPosCallback);
	glfwSetMouseButtonCallback(m_mainWindow, GLFWCallbacks::StaticMouseButtonCallback);
	glfwSetFramebufferSizeCallback(m_mainWindow, GLFWCallbacks::StaticFramebufferSizeCallback);
	glfwSetWindowFocusCallback(m_mainWindow, GLFWCallbacks::StaticWindowFocusCallback);
	glfwSetWindowCloseCallback(m_mainWindow, GLFWCallbacks::StaticWindowCloseCallback);

	m_application = new Core::Engine;
	PE_ENSURE_INIT(result, m_application->Initialize(this, config, m_mainWindow, m_inputSystem),
				   "Failed to initialize application.");
	return ERROR_CODE::OK;
}

void PlatformSystem::Shutdown() {
	Utilities::SafeShutdown(m_application);
	Utilities::SafeShutdown(m_inputSystem);
	Utilities::SafeDelete(m_callbackManager);

	if (m_mainWindow) glfwDestroyWindow(m_mainWindow);

	glfwTerminate();

	return;
}

void PlatformSystem::Run() {
	m_timer.Reset();

	while (!m_appShouldClose) {
		glfwPollEvents();

		m_timer.Tick();

		if (!m_appPaused) {
			const float dt = m_timer.DeltaTime();
			m_inputSystem->OnUpdate(dt);
			m_application->UpdateApplication(dt);
		} else
			glfwWaitEventsTimeout(0.1);
	}
}

void PlatformSystem::RequestToCloseTheApplication() { m_appShouldClose = true; }

void PlatformSystem::CalculateFrameStats() const {
	static int	 frameCnt	 = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	if ((m_timer.TotalTime() - timeElapsed) >= 1.0f) {
		auto  fps  = static_cast<float>(frameCnt);
		float mspf = 1000.0f / fps;

		std::stringstream outs;
		outs.precision(6);
		outs << m_mainWindowCaption << "    " << "FPS: " << fps << "    " << "Frame Time: " << mspf << " (ms)";

		glfwSetWindowTitle(m_mainWindow, outs.str().c_str());

		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void PlatformSystem::OnWindowResize(const int width, const int height) {
	if (m_application && m_application->GetRenderSystem()->GetRenderer()) {
		if (width == 0 || height == 0) {
			m_appPaused = true;
			m_minimized = true;
			m_maximized = false;
		} else {
			m_appPaused						= false;
			m_minimized						= false;
			m_maximized						= glfwGetWindowAttrib(m_mainWindow, GLFW_MAXIMIZED) == GLFW_TRUE;
			ref_config->renderConfig.width	= width;
			ref_config->renderConfig.height = height;
			m_application->GetRenderSystem()->OnResize(ref_config->renderConfig);
		}
	}
}

void PlatformSystem::OnWindowFocus(const int focused) {
	if (focused == GLFW_TRUE) {
		m_appPaused = false;
		m_timer.Start();
	} else {
		m_appPaused = true;
		m_timer.Stop();
	}
}

void PlatformSystem::OnWindowClose() { glfwSetWindowShouldClose(m_mainWindow, GLFW_TRUE); }
}  // namespace PE::Platform