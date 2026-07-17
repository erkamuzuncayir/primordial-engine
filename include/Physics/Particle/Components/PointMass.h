#pragma once
#include "Math/Math.h"

namespace PE::Physics::Particle::Components {
struct PointMass {
	PointMass() = default;
	explicit PointMass(const Math::RVec3 position) : position(position) {}
	PointMass(const Math::RVec3 position, const Math::RVec3 velocity) : position(position), velocity(velocity) {}
	PointMass(const Math::RVec3 position, const Math::RVec3 velocity, const Math::RVec3 acceleration)
		: position(position), velocity(velocity), acceleration(acceleration) {}

	void ClearAccumulator() { forceAccum = Math::RVec3(0.0f); }

	// TODO: Add sleep
	Math::RVec3 position{0, 0, 0};
	Math::RVec3 velocity{0, 0, 0};
	Math::RVec3 acceleration{0, 0, 0};
	Math::RVec3 forceAccum{0, 0, 0};
	Math::real	linearDamping{0.99f};
	Math::real	inverseMass{1};
	Math::real	gravityScale{1.0};
};
}  // namespace PE::Physics::Particle::Components