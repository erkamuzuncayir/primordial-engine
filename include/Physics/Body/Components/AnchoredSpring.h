#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {
// TODO: Incomplete
struct AnchoredSpring {
	AnchoredSpring() = default;
	explicit AnchoredSpring(const uint32_t anchoredRigidBodyIndex, const Math::real springConstant,
							const Math::real restLength)
		: anchoredRigidBodyIndex(anchoredRigidBodyIndex), springConstant(springConstant), restLength(restLength) {}

	// The anchored rigid body of the spring.
	uint32_t anchoredRigidBodyIndex{ECS::INVALID_ENTITY_ID};

	// Holds the spring constant.
	Math::real springConstant{0};

	// Holds the rest length of the spring.
	Math::real restLength{0};
};
}  // namespace PE::Physics::Body::Components