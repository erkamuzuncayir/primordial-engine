#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {

struct KinematicBody {
	KinematicBody() = default;
	explicit KinematicBody(const Math::RVec3 position) : position(position) {}
	KinematicBody(const Math::RVec3 position, const Math::RQuat orientation)
		: position(position), orientation(orientation) {}
	KinematicBody(const Math::RVec3 position, const Math::RVec3 velocity) : position(position), velocity(velocity) {}

	void UpdateDerivedData() { orientation = Math::Normalize(orientation); }

	Math::RVec3 position{0, 0, 0};
	Math::RVec3 targetPosition{0, 0, 0};
	Math::RQuat orientation{0, {0, 0, 0}};
	Math::RQuat targetOrientation{0, {0, 0, 0}};
	Math::RVec3 velocity{0, 0, 0};
	Math::RVec3 angularVelocity{0, 0, 0};
};
}  // namespace PE::Physics::Body::Components