#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {
struct CapsuleCollider {
	CapsuleCollider() = default;
	explicit CapsuleCollider(const Math::real radius) : localRadius(radius) {};
	CapsuleCollider(const Math::real radius, const Math::real halfHeight)
		: localRadius(radius), localHalfHeight(halfHeight) {};
	CapsuleCollider(const Math::real radius, const Math::real halfHeight, const Math::RVec3 offset)
		: localRadius(radius), localHalfHeight(halfHeight), localOffset(offset) {};

	Math::real	localRadius{0.5};
	Math::real	worldRadius{0.5};
	Math::real	localHalfHeight{1.0};
	Math::real	worldHalfHeight{1.0};
	Math::RVec3 localOffset{Math::RVec3Zero};
};
}  // namespace PE::Physics::Body::Components