#pragma once
#include "Math/Math.h"

namespace PE::Graphics::Components {
struct Camera {
	Math::Matrix4 viewMatrix	   = Math::Matrix4Identity();
	Math::Matrix4 projectionMatrix = Math::Matrix4Identity();
	float		  fovY			   = 0;
	float		  aspectRatio	   = 0;
	float		  nearZ			   = 0;
	float		  farZ			   = 0;

	bool isActive = false;
	bool isDirty  = false;
};
}  // namespace PE::Graphics::Components