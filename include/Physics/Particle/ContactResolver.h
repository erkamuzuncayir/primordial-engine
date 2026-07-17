#pragma once

#include "Components/PointMass.h"
#include "Contact.h"
#include "ECS/ECSManager.h"
#include "Math/Math.h"

namespace PE::Physics::Particle {
namespace Systems {
class ParticlePhysicsSystem;
}

class ContactResolver {
public:
	void ResolveContacts(ECS::ECSManager *eM, Systems::ParticlePhysicsSystem *pS, uint32_t iterations, Math::real dt);

private:
	void ResolveVelocity(Components::PointMass *first, Components::PointMass *second, const Contact &contact,
						 Math::real dt);

	void ResolveInterpenetration(Components::PointMass *first, Components::PointMass *second, Contact &contact);

	[[nodiscard]] Math::real CalculateSeparatingVelocity(const Components::PointMass *first,
														 const Components::PointMass *second, Math::RVec3 normal) const;
};
};	// namespace PE::Physics::Particle