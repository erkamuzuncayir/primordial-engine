#pragma once
#include "Math/Math.h"

namespace PE::Physics::Particle::Components {
struct Spring {
	Spring() = default;
	explicit Spring(const uint32_t otherPointMassIndex, const Math::real springConstant, const Math::real restLength)
		: otherPointMassIndex(otherPointMassIndex), springConstant(springConstant), restLength(restLength) {}

	// The point mass at the other end of the spring.
	uint32_t otherPointMassIndex{ECS::INVALID_ENTITY_ID};

	// Holds the spring constant.
	Math::real springConstant{0};

	// Holds the rest length of the spring.
	Math::real restLength{0};
};
}  // namespace PE::Physics::Particle::Components