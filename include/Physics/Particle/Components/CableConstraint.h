#pragma once
#include "Math/Math.h"

namespace PE::Physics::Particle::Components {
struct CableConstraint {
	CableConstraint() = default;
	explicit CableConstraint(const uint32_t first, const uint32_t second) : pointMassIndexes{first, second} {}

	// Holds the pair of point mass indexes that are connected by this link.
	uint32_t pointMassIndexes[2]{ECS::INVALID_ENTITY_ID, ECS::INVALID_ENTITY_ID};

	// Holds the maximum length of the cable.
	Math::real MaxLength{0};

	// Holds the restitution (bounciness) of the cable.
	Math::real Restitution{0};
};
}  // namespace PE::Physics::Particle::Components