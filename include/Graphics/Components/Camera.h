#pragma once
#include "Math/Math.h"

namespace PE::Graphics::Components {
struct Camera {
	Math::Mat44 viewMatrix		 = Math::Mat44Identity();
	Math::Mat44 projectionMatrix = Math::Mat44Identity();
	float		fovY			 = 0;
	float		aspectRatio		 = 0;
	float		nearZ			 = 0;
	float		farZ			 = 0;

	bool isActive = false;
	bool isDirty  = false;
};
}  // namespace PE::Graphics::Components