#pragma once
#include "Math/Math.h"

namespace PE::Graphics::Components {
struct DirectionalLight {
	Math::Vector4 color = Math::Vector4(1, 1, 1, 1);
};
}  // namespace PE::Graphics::Components