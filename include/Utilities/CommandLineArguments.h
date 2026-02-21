#pragma once
#include "Graphics/RenderConfig.h"
#include "Logger.h"

namespace PE::Utilities {
struct CommandLineArguments {
	bool						  isAnyArgumentSet = false;
	bool						  developerMode	   = false;
	bool						  enableLogging	   = false;
	std::string					  logFilePath;
	uint16_t					  logLevel	   = static_cast<uint16_t>(LogLevel::None);
	Graphics::SupportedGraphicAPI graphicAPI   = Graphics::SupportedGraphicAPI::None;
	bool						  enableVSync  = false;
	uint16_t					  clientWidth  = 800;
	uint16_t					  clientHeight = 600;
};
}  // namespace PE::Utilities