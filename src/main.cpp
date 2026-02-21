#include "Platform/PlatformSystem.h"
#include "Utilities/IOUtilities.h"
#include "Utilities/MemoryUtilities.h"

#if defined(_WIN32)
typedef unsigned long DWORD_T;

extern "C" {
__declspec(dllexport) DWORD_T NvOptimusEnablement = 0x00000001;
}

extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

PE::Core::EngineConfig CreateEngineConfig(PE::Core::EngineConfig					&config,
										  const PE::Utilities::CommandLineArguments &args);

int main(int argc, char *argv[]) {
#ifdef _DEBUG
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

	// Enable the automatic leak check at exit
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	PE::Platform::PlatformSystem	   *platform = new PE::Platform::PlatformSystem;
	PE::Utilities::CommandLineArguments args;
	PE::Utilities::IOUtilities::ParseConfigIni("config.ini", args);
	PE::Utilities::IOUtilities::GetCommandlineArguments(argc, argv, args);
	PE::Core::EngineConfig *engineConfig = new PE::Core::EngineConfig();
	CreateEngineConfig(*engineConfig, args);

	if (const auto result = platform->Initialize(*engineConfig); result < PE::ERROR_CODE::WARN_START) {
		PE::Utilities::SafeShutdown(platform);
		PE::Utilities::SafeDelete(engineConfig);
		PE_LOG_FATAL("Platform system can't initialized.");
		return 1;
	} else
		platform->Run();

	PE::Utilities::SafeShutdown(platform);
	PE::Utilities::SafeDelete(engineConfig);

	return 0;
}

PE::Core::EngineConfig CreateEngineConfig(PE::Core::EngineConfig					&config,
										  const PE::Utilities::CommandLineArguments &args) {
	if (!args.isAnyArgumentSet) {
		PE_LOG_WARN("No command line arguments provided.");
		return {};
	}

	config.developerMode						 = args.developerMode;
	config.enableLogging						 = args.enableLogging;
	config.logFilePath							 = args.logFilePath;
	config.logLevel								 = args.logLevel;
	config.renderConfig.graphicAPI				 = args.graphicAPI == PE::Graphics::SupportedGraphicAPI::None
													   ? PE::Graphics::SupportedGraphicAPI::Vulkan
													   : PE::Graphics::SupportedGraphicAPI::D3D11;
	config.renderConfig.enableVSync				 = args.enableVSync;
	config.renderConfig.width					 = args.clientWidth;
	config.renderConfig.height					 = args.clientHeight;
	config.renderConfig.maxCameraCount			 = 1;
	config.renderConfig.maxDirectionalLightCount = 1;
	config.maxEntityCount						 = 1024;
	config.maxComponentTypeCount				 = 1024;

	return config;
}