#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {
struct AeroControl {
	Math::RMat33 baseTensor{0};
	Math::RMat33 minTensor{0};
	Math::RMat33 maxTensor{0};

	Math::RVec3 position{0};

	Math::real controlSetting{0.0};
};
}  // namespace PE::Physics::Body::Components