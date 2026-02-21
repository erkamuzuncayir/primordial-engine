#pragma once
#include <cstdint>

namespace PE::Graphics {
enum class SupportedGraphicAPI { None, D3D11, Vulkan };

enum class RenderPathType { Forward, Deferred };

struct RenderConfig {
	SupportedGraphicAPI graphicAPI		  = SupportedGraphicAPI::Vulkan;
	RenderPathType		renderPath		  = RenderPathType::Forward;
	bool				enableVSync		  = false;
	bool				enable4xMSAA	  = true;
	uint8_t				maxFramesInFlight = 2;
	uint8_t				maxMaterialCount  = 32;
	uint8_t				msaaCount		  = 4;
	// Mutable because those can change afterward
	mutable uint8_t	 maxMsaaQuality			  = 0;
	mutable uint16_t width					  = 1920;
	mutable uint16_t height					  = 1080;
	uint16_t		 maxCameraCount			  = 3;
	uint16_t		 maxDirectionalLightCount = 1;
	uint32_t		 maxParticlesPerFrame	  = 50000;
};
}  // namespace PE::Graphics