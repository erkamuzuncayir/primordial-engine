#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {
struct BoxCollider {
	BoxCollider() = default;
	explicit BoxCollider(const Math::RVec3 halfExtents) : localHalfExtents(halfExtents) {};
	BoxCollider(const Math::RVec3 halfExtents, const Math::RVec3 localOffset)
		: localHalfExtents(halfExtents), localOffset(localOffset) {};

	Math::RVec3 localHalfExtents{0.5, 0.5, 0.5};
	Math::RVec3 worldHalfExtents{0.5, 0.5, 0.5};
	Math::RVec3 localOffset{Math::RVec3Zero};
};
}  // namespace PE::Physics::Body::Components