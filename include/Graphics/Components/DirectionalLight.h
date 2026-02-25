#pragma once
#include "Math/Math.h"

namespace PE::Graphics::Components {
struct DirectionalLight {
	Math::Vec4 color = Math::Vec4(1, 1, 1, 1);
};
}  // namespace PE::Graphics::Components