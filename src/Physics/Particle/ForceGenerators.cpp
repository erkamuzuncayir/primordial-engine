#include "Physics/Particle/ForceGenerators.h"

#include "Physics/Core/ForceApplicator.h"
#include "Physics/Particle/Components/AnchoredBungee.h"
#include "Physics/Particle/Components/AnchoredSpring.h"
#include "Physics/Particle/Components/Buoyancy.h"
#include "Physics/Particle/Components/Drag.h"
#include "Physics/Particle/Components/PointMass.h"
#include "Physics/Particle/Components/Spring.h"

namespace PE::Physics::Particle {

void ForceGenerators::UpdateForces(ECS::ECSManager *eM) {
	UpdateAnchoredBungeeForces(eM);
	UpdateAnchoredSpringForces(eM);
	UpdateBuoyancyForces(eM);
	UpdateDragForces(eM);
	UpdateSpringForces(eM);
}

void ForceGenerators::UpdateAnchoredBungeeForces(ECS::ECSManager *eM) {
	// Calculate the vector of the spring.
	auto		&anchoredBungeeCompArr = eM->GetCompArr<Components::AnchoredBungee>();
	const auto	&anchoredBungees	   = anchoredBungeeCompArr.Data();
	const size_t count				   = anchoredBungees.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId = anchoredBungeeCompArr.Index()[i];
		auto		  &currAB	= anchoredBungees[i];
		const auto	   currPM	= eM->GetTComponent<Components::PointMass>(entityId);
		if (!currPM) {
			PE_LOG_WARN("Self PointMass is missing!");
			continue;
		}
		const auto	  &otherPM	= eM->GetTComponent<Components::PointMass>(currAB.otherPointMassIndex);
		if (!otherPM) {
			PE_LOG_WARN("Other PointMass is missing!");
			continue;
		}

		// Calculate the vector of the spring.
		Math::RVec3 force = currPM->position - otherPM->position;

		Math::real magnitude = Math::Length(force);

		// Check if the bungee is compressed.
		if (magnitude <= currAB.restLength) continue;

		// Calculate the magnitude of the force.
		magnitude -= currAB.restLength;
		magnitude *= currAB.springConstant;

		// Calculate the final force and apply to just one side.
		force = Math::Normalize(force) * -magnitude;
		Core::ForceApplicator::ApplyForce(*currPM, force);
	}
}

void ForceGenerators::UpdateAnchoredSpringForces(ECS::ECSManager *eM) {
	// Calculate the vector of the spring.
	auto		&anchoredSpringCompArr = eM->GetCompArr<Components::AnchoredSpring>();
	const auto	&anchoredSprings	   = anchoredSpringCompArr.Data();
	const size_t count				   = anchoredSprings.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId = anchoredSpringCompArr.Index()[i];
		auto		  &currAS	= anchoredSprings[i];
		const auto	   currPM	= eM->GetTComponent<Components::PointMass>(entityId);
		if (!currPM) {
			PE_LOG_WARN("Self PointMass is missing!");
			continue;
		}
		const auto	  &otherPM	= eM->GetTComponent<Components::PointMass>(currAS.anchoredPointMassIndex);
		if (!otherPM) {
			PE_LOG_WARN("Other PointMass is missing!");
			continue;
		}

		// Calculate the vector of the spring.
		Math::RVec3 force = currPM->position - otherPM->position;

		// Calculate the magnitude of the force.
		// (Hooke's law: F = -k * Δx)
		const Math::real magnitude = currAS.springConstant * (Math::Length(force) - currAS.restLength);

		// Calculate the final force and apply to just one side.
		force = Math::Normalize(force) * -magnitude;
		currPM->forceAccum += force;
	}
}

void ForceGenerators::UpdateBuoyancyForces(ECS::ECSManager *eM) {
	// Calculate the vector of the spring.
	auto		&buoyancyCompArr = eM->GetCompArr<Components::Buoyancy>();
	const auto	&buoyancies		 = buoyancyCompArr.Data();
	const size_t count			 = buoyancies.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId		= buoyancyCompArr.Index()[i];
		auto		  &currBuoyancy = buoyancies[i];
		const auto	   currPM		= eM->GetTComponent<Components::PointMass>(entityId);
		if (!currPM) {
			PE_LOG_WARN("Self PointMass is missing!");
			continue;
		}

		// Calculate the submersion depth.
		Math::real depth = currPM->position.y;

		// Check if we’re out of the water.
		if (depth >= currBuoyancy.waterHeight + currBuoyancy.maxDepth) continue;

		Math::RVec3 force(0, 0, 0);
		// Check if we’re at maximum depth.
		if (depth <= currBuoyancy.waterHeight - currBuoyancy.maxDepth) {
			force.y = currBuoyancy.liquidDensity * currBuoyancy.volume;
			currPM->forceAccum += force;
			continue;
		}
		// Otherwise we are partly submerged.
		force.y = currBuoyancy.liquidDensity * currBuoyancy.volume *
				  (currBuoyancy.waterHeight + currBuoyancy.maxDepth - depth) / (2 * currBuoyancy.maxDepth);
		currPM->forceAccum += force;
	}
}

void ForceGenerators::UpdateDragForces(ECS::ECSManager *eM) {
	auto		&dragCompArr = eM->GetCompArr<Components::Drag>();
	const auto	&drags		 = dragCompArr.Data();
	const size_t count		 = drags.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId = dragCompArr.Index()[i];
		const auto	   currPM	= eM->GetTComponent<Components::PointMass>(entityId);
		if (!currPM) {
			PE_LOG_WARN("Self PointMass is missing!");
			continue;
		}

		Math::RVec3	   force	= currPM->velocity;

		// Calculate the total drag coefficient.
		Math::real dragCoeff = Math::Length(force);
		if (dragCoeff <= Math::REpsilon) {
			continue;
		}
		dragCoeff = drags[i].k1 * dragCoeff + drags[i].k2 * dragCoeff * dragCoeff;

		// Calculate the final force and apply it.
		force = Math::Normalize(force);
		force *= -dragCoeff;
		Core::ForceApplicator::ApplyForce(*currPM, force);
	}
}

void ForceGenerators::UpdateSpringForces(ECS::ECSManager *eM) {
	// Calculate the vector of the spring.
	auto		&springCompArr = eM->GetCompArr<Components::Spring>();
	const auto	&springs	   = springCompArr.Data();
	const size_t count		   = springs.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId	  = springCompArr.Index()[i];
		auto		  &currSpring = springs[i];
		const auto	   currPM	  = eM->GetTComponent<Components::PointMass>(entityId);
		if (!currPM) {
			PE_LOG_WARN("Self PointMass is missing!");
			continue;
		}
		const auto	  &otherPM	  = eM->GetTComponent<Components::PointMass>(currSpring.otherPointMassIndex);
		if (!otherPM) {
			PE_LOG_WARN("Other PointMass is missing!");
			continue;
		}

		// Calculate the vector of the spring.
		Math::RVec3 force = currPM->position - otherPM->position;

		// Calculate the magnitude of the force.
		// (Hooke's law: F = -k * Δx)
		const Math::real magnitude = currSpring.springConstant * (Math::Length(force) - currSpring.restLength);

		// Calculate the final force and apply to both sides.
		force = Math::Normalize(force) * -magnitude;
		Core::ForceApplicator::ApplyForce(*currPM, force);
		Core::ForceApplicator::ApplyForce(*otherPM, -force);
	}
}
}  // namespace PE::Physics::Particle