#pragma once
#include <vector>

#include "Graphics/RenderTypes.h"

namespace PE::Graphics::Components {
struct MeshRenderer {
	struct SubMeshInfo {
		MeshID	   meshID	  = INVALID_HANDLE;
		MaterialID materialID = INVALID_HANDLE;
	};

	std::vector<SubMeshInfo> subMeshes;
	bool					 isVisible		  = true;
	bool					 forceTransparent = false;
	bool					 castShadows	  = true;
	bool					 receiveShadows	  = true;
};
}  // namespace PE::Graphics::Components