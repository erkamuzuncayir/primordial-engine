
#include "Physics/Particle/ContactResolver.h"

#include "Physics/Particle/Components/PointMass.h"
#include "Physics/Particle/Systems/ParticlePhysicsSystem.h"

namespace PE::Physics::Particle {
void ContactResolver::ResolveContacts(ECS::ECSManager *eM, Systems::ParticlePhysicsSystem *pS,
									  const uint32_t iterations, const Math::real dt) {
	auto		&contacts	 = pS->GetContacts();
	const size_t numContacts = contacts.size();
	if (numContacts == 0) return;

	Contact				  *selContact = nullptr;
	Components::PointMass *selFirst	  = nullptr;
	Components::PointMass *selSecond  = nullptr;

	for (uint32_t iterationsUsed = 0; iterationsUsed < iterations; ++iterationsUsed) {
		// Find the contact with the largest closing velocity.
		Math::real max		= Math::RMax;
		size_t	   maxIndex = numContacts;
		for (uint32_t i = 0; i < numContacts; i++) {
			Contact	  &contact = contacts[i];
			const auto first   = eM->GetTComponent<Components::PointMass>(contact.pointMassIndexes[0]);
			const auto second  = eM->GetTComponent<Components::PointMass>(contact.pointMassIndexes[1]);

			const Math::real sepVel = CalculateSeparatingVelocity(first, second, contact.normal);
			if (sepVel < max && (sepVel < 0 || contact.penetration > 0)) {
				max		   = sepVel;
				selContact = &contact;
				selFirst   = first;
				selSecond  = second;
				maxIndex   = i;
			}
		}
		// Do we have anything worth resolving?
		if (maxIndex == numContacts) break;

		// Resolve this contact.
		ResolveVelocity(selFirst, selSecond, *selContact, dt);

		// If we don’t have any penetration, skip this step.
		if (selContact->penetration > 0) {
			ResolveInterpenetration(selFirst, selSecond, *selContact);

			// TODO: Chapter 16 add additional code here
		}
	}
}

void ContactResolver::ResolveVelocity(Components::PointMass *first, Components::PointMass *second,
									  const Contact &contact, const Math::real dt) {
	// Find the velocity in the direction of the contact.
	const Math::real separatingVel = CalculateSeparatingVelocity(first, second, contact.normal);

	// Check if it needs to be resolved.
	if (separatingVel > 0) {
		// The contact is either separating, or stationary; no impulse is required.
		return;
	}

	// Calculate the new separating velocity.
	Math::real newSepVel = -separatingVel * contact.restitution;

	// Check the if velocity buildup due to acceleration only.
	Math::RVec3 accCausedVel = first->acceleration;
	if (second) accCausedVel -= second->acceleration;

	// If we’ve got a closing velocity due to acceleration buildup, remove it from the new separating velocity.
	if (const Math::real accCausedSepVel = Math::Dot(accCausedVel, contact.normal) * dt; accCausedSepVel < 0) {
		newSepVel += contact.restitution * accCausedSepVel;
		// Make sure we haven’t removed more than was there to remove.
		if (newSepVel < 0) newSepVel = 0;
	}

	const Math::real deltaVel = newSepVel - separatingVel;

	// We apply the change in velocity to each object in proportion to
	// their inverse mass (i.e., those with lower inverse mass [higher
	// actual mass] get less change in velocity).
	Math::real totalInverseMass = first->inverseMass;
	if (second) totalInverseMass += second->inverseMass;

	// If all particles have infinite mass, then impulses have no effect.
	if (totalInverseMass <= 0) return;

	// Calculate the impulse to apply.
	const Math::real impulse = deltaVel / totalInverseMass;

	// Find the amount of impulse per unit of inverse mass.
	const Math::RVec3 impulsePerInverseMass = contact.normal * impulse;

	// Apply impulses: they are applied in the direction of the contact,
	// and are proportional to the inverse mass.
	first->velocity += impulsePerInverseMass * first->inverseMass;

	// Second particle goes in the opposite direction
	if (second) second->velocity -= impulsePerInverseMass * second->inverseMass;
}

void ContactResolver::ResolveInterpenetration(Components::PointMass *first, Components::PointMass *second,
											  Contact &contact) {
	// The movement of each object is based on their inverse mass, so total that.
	Math::real totalInverseMass = first->inverseMass;
	if (second) totalInverseMass += second->inverseMass;

	// If all particles have infinite mass, then we do nothing.
	if (totalInverseMass <= 0) return;

	// Find the amount of penetration resolution per unit of inverse mass.
	const Math::RVec3 movePerIMass = contact.normal * (contact.penetration / totalInverseMass);

	// Calculate the movement amounts.
	contact.particleMovements[0] = movePerIMass * first->inverseMass;
	second ? contact.particleMovements[1] = movePerIMass * -second->inverseMass
		   : contact.particleMovements[1] = Math::RVec3Zero;

	// Apply the penetration resolution.
	first->position += contact.particleMovements[0];
	if (second) second->position += contact.particleMovements[1];

	contact.penetration = 0;
}

Math::real ContactResolver::CalculateSeparatingVelocity(const Components::PointMass *first,
														const Components::PointMass *second,
														const Math::RVec3			 normal) const {
	Math::RVec3 relVel = first->velocity;
	if (second) relVel -= second->velocity;

	return Math::Dot(relVel, normal);
}
}  // namespace PE::Physics::Particle