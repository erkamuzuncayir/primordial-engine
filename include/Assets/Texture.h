#pragma once
#include <vector>

#include "Graphics/Texture.h"
#include "Utilities/Logger.h"

namespace PE::Assets::Texture {
namespace Generator {
static std::vector<unsigned char> GetDefaultTexture(const Graphics::TextureType type) {
	switch (type) {
		case Graphics::TextureType::Albedo:
		case Graphics::TextureType::Terrain_Layer_0:
		case Graphics::TextureType::Terrain_Layer_1:
		case Graphics::TextureType::Terrain_Layer_2:
		case Graphics::TextureType::Terrain_Layer_3: return {255, 255, 255, 255}; break;
		case Graphics::TextureType::Normal: return {128, 128, 255, 255}; break;
		case Graphics::TextureType::Mask: return {255, 255, 0, 255}; break;
		case Graphics::TextureType::Emissive:
		case Graphics::TextureType::Displacement:
		case Graphics::TextureType::Generic_0:
		case Graphics::TextureType::Generic_1: return {0, 0, 255, 255}; break;
		case Graphics::TextureType::Terrain_Splat: return {0, 0, 0, 0}; break;
		default:
			PE_LOG_ERROR("Unknown texture type");
			return {0, 0, 0, 0};
			break;
	}
}

static std::vector<unsigned char> GetDefaultError() { return {255, 0, 255, 255}; }
}  // namespace Generator

namespace Loader {
bool Load(const std::filesystem::path &path, Graphics::Texture &texture);
bool LoadCubemap(const std::vector<std::filesystem::path> &paths, Graphics::Texture &texture);
};	// namespace Loader
}  // namespace PE::Assets::Texture