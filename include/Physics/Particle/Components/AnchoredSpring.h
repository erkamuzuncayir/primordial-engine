#pragma once
#include "Math/Math.h"

namespace PE::Physics::Particle::Components {
struct AnchoredSpring {
	AnchoredSpring() = default;
	explicit AnchoredSpring(const uint32_t anchoredPointMassIndex, const Math::real springConstant,
							const Math::real restLength)
		: anchoredPointMassIndex(anchoredPointMassIndex), springConstant(springConstant), restLength(restLength) {}

	// The anchored point mass of the spring.
	uint32_t anchoredPointMassIndex{ECS::INVALID_ENTITY_ID};

	// Holds the spring constant.
	Math::real springConstant{0};

	// Holds the rest length of the spring.
	Math::real restLength{0};
};
}  // namespace PE::Physics::Particle::Components