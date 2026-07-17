#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {
struct SphereCollider {
	SphereCollider() = default;
	explicit SphereCollider(const Math::real radius) : localRadius(radius) {};
	SphereCollider(const Math::real radius, const Math::RVec3 offset) : localRadius(radius), localOffset(offset) {};

	Math::real	localRadius{0.5};
	Math::real	worldRadius{0.5};
	Math::RVec3 localOffset{Math::RVec3Zero};
};
}  // namespace PE::Physics::Body::Components