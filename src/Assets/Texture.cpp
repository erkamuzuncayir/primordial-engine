#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <vector>

#ifdef PE_D3D11
#include <d3d11.h>
#include <dxgiformat.h>
#endif

#include <Assets/Texture.h>
#include <Utilities/Logger.h>

#include "Graphics/D3D11/D3D11Types.h"

namespace PE::Assets::Texture::Loader {
bool Load(const std::filesystem::path &path, Graphics::Texture &texture) {
	const std::string pathStr = path.string();
	int				  width;
	int				  height;
	int				  texChannels;

	stbi_uc *pixels = stbi_load(pathStr.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);

	if (!pixels) {
		PE_LOG_ERROR("Failed to load image: " + pathStr + " Reason: " + stbi_failure_reason());
		return false;
	}

	auto &params	   = texture.GetTextureParameters();
	params.width	   = static_cast<uint16_t>(width);
	params.height	   = static_cast<uint16_t>(height);
	params.depth	   = 32;
	params.mipLevels   = 1;
	params.arrayLayers = 1;
	params.samples	   = 1;

	bool isSRGB;
	switch (params.type) {
		case Graphics::TextureType::Normal:
		case Graphics::TextureType::Mask:
		case Graphics::TextureType::Displacement:
		case Graphics::TextureType::Terrain_Splat:
		case Graphics::TextureType::Generic_0:
		case Graphics::TextureType::Generic_1: isSRGB = false; break;
		case Graphics::TextureType::Albedo:
		case Graphics::TextureType::Emissive:
		case Graphics::TextureType::Terrain_Layer_0:
		case Graphics::TextureType::Terrain_Layer_1:
		case Graphics::TextureType::Terrain_Layer_2:
		case Graphics::TextureType::Terrain_Layer_3:
		default: isSRGB = true; break;
	}

#ifdef PE_D3D11
	params.format.dxgiFormat = isSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

	params.usage.d3d11Usage = D3D11_USAGE_DEFAULT;
#elif PE_VULKAN
	params.format.vulkanFormat = isSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

	params.usage.vulkanUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
#endif

	auto &textureData = texture.GetTextureData();

	const size_t imageSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;

	textureData.resize(imageSize);

	std::memcpy(textureData.data(), pixels, imageSize);

	stbi_image_free(pixels);

	return true;
}

bool LoadCubemap(const std::vector<std::filesystem::path> &paths, Graphics::Texture &texture) {
	if (paths.size() != 6) {
		PE_LOG_ERROR("Cubemap loading requires exactly 6 file paths.");
		return false;
	}

	int	  width		  = 0;
	int	  height	  = 0;
	auto &textureData = texture.GetTextureData();
	textureData.clear();

	for (size_t i = 0; i < 6; ++i) {
		const std::string pathStr = paths[i].string();
		int				  w, h, c;

		stbi_uc *pixels = stbi_load(pathStr.c_str(), &w, &h, &c, STBI_rgb_alpha);

		if (!pixels) {
			PE_LOG_ERROR("Failed to load cubemap face: " + pathStr);
			return false;
		}

		if (i == 0) {
			width  = w;
			height = h;

			const size_t faceSize = static_cast<size_t>(width) * height * 4;
			textureData.resize(faceSize * 6);
		} else {
			if (w != width || h != height) {
				PE_LOG_ERROR("Cubemap face dimension mismatch! All faces must be same size.");
				stbi_image_free(pixels);
				return false;
			}
		}

		const size_t faceSize = static_cast<size_t>(width) * height * 4;
		std::memcpy(textureData.data() + (i * faceSize), pixels, faceSize);

		stbi_image_free(pixels);
	}

	auto &params	   = texture.GetTextureParameters();
	params.width	   = static_cast<uint16_t>(width);
	params.height	   = static_cast<uint16_t>(height);
	params.depth	   = 32;
	params.mipLevels   = 1;
	params.arrayLayers = 6;
	params.samples	   = 1;

	bool isSRGB;
	switch (params.type) {
		case Graphics::TextureType::Mask:
		case Graphics::TextureType::Displacement:
		case Graphics::TextureType::Terrain_Splat:
		case Graphics::TextureType::Generic_0:
		case Graphics::TextureType::Generic_1: isSRGB = false; break;
		case Graphics::TextureType::Albedo:
		case Graphics::TextureType::Normal:
		case Graphics::TextureType::Emissive:
		case Graphics::TextureType::Terrain_Layer_0:
		case Graphics::TextureType::Terrain_Layer_1:
		case Graphics::TextureType::Terrain_Layer_2:
		case Graphics::TextureType::Terrain_Layer_3:
		default: isSRGB = true; break;
	}

#ifdef PE_D3D11
	params.format.dxgiFormat = isSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;

	params.usage.d3d11Usage = D3D11_USAGE_DEFAULT;
#elif PE_VULKAN
	params.format.vulkanFormat = isSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	params.usage.vulkanUsage   = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
#endif

	return true;
}
}  // namespace PE::Assets::Texture::Loader