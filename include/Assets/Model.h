#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "Assets/AssetInfo.h"
#include "Graphics/RenderTypes.h"

namespace PE::Assets::Model {
namespace Loader {
struct ProcessedMesh {
	MeshAssetInfo	   assetInfo;
	Graphics::MeshData meshData;
};

struct ModelLoadResult {
	ModelAssetInfo				   modelAssetInfo;
	std::vector<ProcessedMesh>	   meshes;
	std::vector<MaterialAssetInfo> materials;
	std::vector<TextureAssetInfo>  textures;
	bool						   success = false;
};

ModelLoadResult LoadOBJ(const std::filesystem::path &path);
bool LoadMTL(const std::filesystem::path &mtlPath, const std::string &modelNamePrefix, ModelLoadResult &outResult);
};	// namespace Loader
}  // namespace PE::Assets::Model