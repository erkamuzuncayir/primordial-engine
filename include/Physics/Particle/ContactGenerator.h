#pragma once

#include "ECS/ECSManager.h"

namespace PE::Physics::Particle::Systems {
class ParticlePhysicsSystem;
}

namespace PE::Physics::Particle {
class ContactGenerator {
public:
	uint32_t GenerateContacts(ECS::ECSManager *eM, Systems::ParticlePhysicsSystem *pS) const;

private:
	uint32_t GenerateCableConstraintContacts(ECS::ECSManager *eM, Systems::ParticlePhysicsSystem *pS) const;
	uint32_t GenerateRodConstraintContacts(ECS::ECSManager *eM, Systems::ParticlePhysicsSystem *pS) const;
};
}  // namespace PE::Physics::Particle