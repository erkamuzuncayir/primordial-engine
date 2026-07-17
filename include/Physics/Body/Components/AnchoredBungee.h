#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {
// TODO: Incomplete
struct AnchoredBungee {
	AnchoredBungee() = default;
	explicit AnchoredBungee(const uint32_t otherRigidBodyIndex, const Math::real springConstant,
							const Math::real restLength)
		: otherRigidBodyIndex(otherRigidBodyIndex), springConstant(springConstant), restLength(restLength) {}

	// The rigid body at the other end of the spring.
	uint32_t otherRigidBodyIndex{ECS::INVALID_ENTITY_ID};

	// Holds the spring constant.
	Math::real springConstant{0};

	// Holds the rest length of the spring.
	Math::real restLength{0};
};
}  // namespace PE::Physics::Body::Components