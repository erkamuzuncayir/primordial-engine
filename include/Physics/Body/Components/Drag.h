#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {
// TODO: Incomplete
struct Drag {
	Drag() = default;
	explicit Drag(const Math::real k1, const Math::real k2) : k1(k1), k2(k2) {}

	Math::real k1{0};
	Math::real k2{0};
};
}  // namespace PE::Physics::Body::Components