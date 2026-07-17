#pragma once
#include "ECS/Entity.h"
#include "Math/Math.h"

namespace PE::Physics::Particle {

struct Contact {
	Math::RVec3 particleMovements[2]{Math::RVec3Zero, Math::RVec3Zero};

	// Holds the direction of the contact in world coordinates.
	Math::RVec3 normal{0, 0, 0};

	/**
	 * Holds the particles that are involved in the contact. The
	 * second of these can be ECS::INVALID_ENTITY_ID for contacts with the scenery.
	 */
	uint32_t pointMassIndexes[2]{ECS::INVALID_ENTITY_ID, ECS::INVALID_ENTITY_ID};

	// Holds the depth of penetration at the contact.
	Math::real penetration;

	// Holds the normal restitution coefficient at the contact.
	Math::real restitution{0.0};
};
}  // namespace PE::Physics::Particle