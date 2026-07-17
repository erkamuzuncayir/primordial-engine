#pragma once
#include "Math/Math.h"

namespace PE::Graphics::Components {
enum class CameraType : uint8_t { Perspective, Orthographic };

struct Camera {
	Math::Mat44 viewMatrix		 = Math::Mat44Identity();
	Math::Mat44 projectionMatrix = Math::Mat44Identity();
	float		aspectRatio		 = 0;
	float		nearZ			 = 0.1;
	float		farZ			 = 1000;

	float fovY		= 60;
	float orthoSize = 10;

	CameraType type		= CameraType::Perspective;
	bool	   isActive = false;
	bool	   isDirty	= true;
};
}  // namespace PE::Graphics::Components