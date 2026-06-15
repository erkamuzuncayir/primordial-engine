#pragma once
#include <vector>

#include "Graphics/RenderTypes.h"

namespace PE::Graphics::Components {
struct MeshRenderer {
	struct SubMeshInfo {
		MeshID	   meshID{};
		MaterialID materialID{};
	};

	std::vector<SubMeshInfo> subMeshes;
	bool					 isVisible		  = true;
	bool					 forceTransparent = false;
	bool					 castShadows	  = true;
	bool					 receiveShadows	  = true;
};
}  // namespace PE::Graphics::Components