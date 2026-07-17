#include "Physics/Particle/ContactGenerator.h"

#include "Physics/Particle/Components/CableConstraint.h"
#include "Physics/Particle/Components/PointMass.h"
#include "Physics/Particle/Components/RodConstraint.h"
#include "Physics/Particle/Contact.h"
#include "Physics/Particle/Systems/ParticlePhysicsSystem.h"

namespace PE::Physics::Particle {
uint32_t ContactGenerator::GenerateContacts(ECS::ECSManager *eM, Systems::ParticlePhysicsSystem *pS) const {
	uint32_t contactCount = GenerateCableConstraintContacts(eM, pS);
	contactCount += GenerateRodConstraintContacts(eM, pS);
	return contactCount;
}

uint32_t ContactGenerator::GenerateCableConstraintContacts(ECS::ECSManager				  *eM,
														   Systems::ParticlePhysicsSystem *pS) const {
	// TODO: Add maxContact to somewhere, preferably physics system config
	const auto	&cables				   = eM->GetCompArr<Components::CableConstraint>().Data();
	const size_t count				   = cables.size();
	int32_t		 generatedContactCount = 0;
	for (uint32_t i = 0; i < count; ++i) {
		const Components::CableConstraint cable	 = cables[i];
		const auto						  first	 = eM->GetTComponent<Components::PointMass>(cable.pointMassIndexes[0]);
		const auto						 *second = eM->GetTComponent<Components::PointMass>(cable.pointMassIndexes[1]);

		const Math::real actualLength = Math::Length(first->position - second->position);
		if (actualLength < cable.MaxLength) {
			continue;
		}

		// Calculate the normal.
		const Math::RVec3 normal	  = Math::Normalize(second->position - first->position);
		const Math::real  penetration = actualLength - cable.MaxLength;
		const Math::real  restitution = cable.Restitution;

		Contact contact{.normal			  = normal,
						.pointMassIndexes = {cable.pointMassIndexes[0], cable.pointMassIndexes[1]},
						.penetration	  = penetration,
						.restitution	  = restitution};
		pS->GetContacts().emplace_back(contact);
		++generatedContactCount;
	}

	return generatedContactCount;
}

uint32_t ContactGenerator::GenerateRodConstraintContacts(ECS::ECSManager				*eM,
														 Systems::ParticlePhysicsSystem *pS) const {
	// TODO: Add maxContact to somewhere, preferably physics system config
	const auto	&rods				   = eM->GetCompArr<Components::RodConstraint>().Data();
	const size_t count				   = rods.size();
	int32_t		 generatedContactCount = 0;
	for (uint32_t i = 0; i < count; ++i) {
		const Components::RodConstraint rod	   = rods[i];
		const auto						first  = eM->GetTComponent<Components::PointMass>(rod.pointMassIndexes[0]);
		const auto					   *second = eM->GetTComponent<Components::PointMass>(rod.pointMassIndexes[1]);

		const Math::real actualLength = Math::Length(first->position - second->position);
		if (rod.Length == actualLength) {
			continue;
		}

		// Calculate the normal.
		Math::RVec3 normal = Math::Normalize(second->position - first->position);
		Math::real	penetration;

		// The contact normal depends on whether we’re extending or compressing.
		if (actualLength > rod.Length) {
			penetration = actualLength - rod.Length;
		} else {
			normal *= -1;
			penetration = rod.Length - actualLength;
		}

		constexpr Math::real restitution = 0;

		Contact contact{.normal			  = normal,
						.pointMassIndexes = {rod.pointMassIndexes[0], rod.pointMassIndexes[1]},
						.penetration	  = penetration,
						.restitution	  = restitution};
		pS->GetContacts().emplace_back(contact);
		++generatedContactCount;
	}
	return generatedContactCount;
}
}  // namespace PE::Physics::Particle