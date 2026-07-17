#pragma once
#include <string>

#include "Graphics/RenderConfig.h"
#include "Utilities/Logger.h"

namespace PE::Core {
struct EngineConfig {
	bool				   developerMode = false;
	bool				   enableLogging = false;
	std::string			   logFilePath;
	uint16_t			   logLevel = static_cast<uint16_t>(Utilities::LogLevel::None);
	Graphics::RenderConfig renderConfig;
	uint16_t			   maxEntityCount		 = 16384;
	uint16_t			   maxComponentTypeCount = 64;
};
}  // namespace PE::Core