#pragma once

namespace PE::Physics::Particle::Components {
struct RodConstraint {
	RodConstraint() = default;
	explicit RodConstraint(const uint32_t first, const uint32_t second) : pointMassIndexes{first, second} {}

	// Holds the pair of particle indexes that are connected by this link.
	uint32_t pointMassIndexes[2]{ECS::INVALID_ENTITY_ID, ECS::INVALID_ENTITY_ID};

	// Holds the length of the rod.
	Math::real Length{0};
};
}  // namespace PE::Physics::Particle::Components