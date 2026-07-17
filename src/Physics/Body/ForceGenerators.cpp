#include "Physics/Body/ForceGenerators.h"

#include "Physics/Body/Components/Aero.h"
#include "Physics/Body/Components/AeroControl.h"
#include "Physics/Body/Components/AnchoredBungee.h"
#include "Physics/Body/Components/AnchoredSpring.h"
#include "Physics/Body/Components/AngledAero.h"
#include "Physics/Body/Components/Buoyancy.h"
#include "Physics/Body/Components/Drag.h"
#include "Physics/Body/Components/RigidBody.h"
#include "Physics/Body/Components/Spring.h"
#include "Physics/Core/ForceApplicator.h"

namespace PE::Physics::Body {

void ForceGenerators::UpdateForces(ECS::ECSManager *eM) {
	UpdateAnchoredBungeeForces(eM);
	UpdateAnchoredSpringForces(eM);
	UpdateBuoyancyForces(eM);
	UpdateDragForces(eM);
	UpdateSpringForces(eM);
	UpdateAeroForces(eM);
	UpdateAeroControlForces(eM);
	UpdateAngledAeroForces(eM, windSpeed);
}

// TODO: Incomplete
void ForceGenerators::UpdateAnchoredBungeeForces(ECS::ECSManager *eM) {
	// Calculate the vector of the spring.
	auto		&anchoredBungeeCompArr = eM->GetCompArr<Components::AnchoredBungee>();
	const auto	&anchoredBungees	   = anchoredBungeeCompArr.Data();
	const size_t count				   = anchoredBungees.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId			  = anchoredBungeeCompArr.Index()[i];
		auto		  &currAnchoredBungee = anchoredBungees[i];
		if (entityId == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		} else if (currAnchoredBungee.otherRigidBodyIndex == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Other RigidBody is missing!");
			continue;
		}

		const auto	   currRb			  = eM->GetTComponent<Components::RigidBody>(entityId);
		if (!currRb) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}
		if (!currRb->isAwake) continue;

		const auto	  &otherRb = eM->GetTComponent<Components::RigidBody>(currAnchoredBungee.otherRigidBodyIndex);
		if (!eM->GetTComponent<Components::RigidBody>(currAnchoredBungee.otherRigidBodyIndex)) {
			PE_LOG_WARN("Other RigidBody is missing!");
			continue;
		}

		// Calculate the vector of the spring.
		Math::RVec3 force = currRb->position - otherRb->position;

		Math::real magnitude = Math::Length(force);

		// Check if the bungee is compressed.
		if (magnitude <= currAnchoredBungee.restLength) continue;

		// Calculate the magnitude of the force.
		magnitude -= currAnchoredBungee.restLength;
		magnitude *= currAnchoredBungee.springConstant;

		// Calculate the final force and apply to just one side.
		force = Math::Normalize(force) * -magnitude;
		currRb->forceAccum += force;
	}
}

// TODO: Incomplete
void ForceGenerators::UpdateAnchoredSpringForces(ECS::ECSManager *eM) {
	// Calculate the vector of the spring.
	auto		&anchoredSpringCompArr = eM->GetCompArr<Components::AnchoredSpring>();
	const auto	&anchoredSprings	   = anchoredSpringCompArr.Data();
	const size_t count				   = anchoredSprings.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId			  = anchoredSpringCompArr.Index()[i];
		auto		  &currAnchoredSpring = anchoredSprings[i];
		if (entityId == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		} else if (currAnchoredSpring.anchoredRigidBodyIndex == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Other RigidBody is missing!");
			continue;
		}

		const auto	   currRb			  = eM->GetTComponent<Components::RigidBody>(entityId);
		if (!currRb) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}
		if (!currRb->isAwake) continue;

		const auto	  &otherRb = eM->GetTComponent<Components::RigidBody>(currAnchoredSpring.anchoredRigidBodyIndex);
		// Calculate the vector of the spring.
		Math::RVec3 force = currRb->position - otherRb->position;

		// Calculate the magnitude of the force.
		// (Hooke's law: F = -k * Δx)
		const Math::real magnitude = currAnchoredSpring.springConstant * (Math::Length(force) - currAnchoredSpring.restLength);

		// Calculate the final force and apply to just one side.
		force = Math::Normalize(force) * -magnitude;
		currRb->forceAccum += force;
	}
}

// TODO: Incomplete
void ForceGenerators::UpdateBuoyancyForces(ECS::ECSManager *eM) {
	// Calculate the vector of the spring.
	auto		&buoyancyCompArr = eM->GetCompArr<Components::Buoyancy>();
	const auto	&buoyancies		 = buoyancyCompArr.Data();
	const size_t count			 = buoyancies.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId = buoyancyCompArr.Index()[i];
		auto		  &buoyancy = buoyancies[i];
		if (entityId == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}

		const auto	   currRb			  = eM->GetTComponent<Components::RigidBody>(entityId);
		if (!currRb) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}
		if (!currRb->isAwake) continue;

		// Calculate the submersion depth.
		Math::real depth = currRb->position.y;

		// Check if we’re out of the water.
		if (depth >= buoyancy.waterHeight + buoyancy.maxDepth) continue;

		Math::RVec3 force(0, 0, 0);
		// Check if we’re at maximum depth.
		if (depth <= buoyancy.waterHeight - buoyancy.maxDepth) {
			force.y = buoyancy.liquidDensity * buoyancy.volume;
			currRb->forceAccum += force;
			continue;
		}
		// Otherwise we are partly submerged.
		force.y = buoyancy.liquidDensity * buoyancy.volume * 
				  (buoyancy.waterHeight + buoyancy.maxDepth - depth) / (2 * buoyancy.maxDepth);
		currRb->forceAccum += force;
	}
}

// TODO: Incomplete
void ForceGenerators::UpdateDragForces(ECS::ECSManager *eM) {
	auto		&dragCompArr = eM->GetCompArr<Components::Drag>();
	const auto	&drags		 = dragCompArr.Data();
	const size_t count		 = drags.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId	 = dragCompArr.Index()[i];
		if (entityId == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}
		const auto	   currRb			  = eM->GetTComponent<Components::RigidBody>(entityId);
		if (!currRb) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}
		if (!currRb->isAwake) continue;

		Math::RVec3	   force	 = currRb->velocity;

		// Calculate the total drag coefficient.
		Math::real dragCoeff = Math::Length(force);
		if (dragCoeff <= Math::REpsilon) {
			continue;
		}
		dragCoeff = drags[i].k1 * dragCoeff + drags[i].k2 * dragCoeff * dragCoeff;

		// Calculate the final force and apply it.
		force = Math::Normalize(force);
		force *= -dragCoeff;
		currRb->forceAccum += force;
	}
}

// TODO: Incomplete
void ForceGenerators::UpdateSpringForces(ECS::ECSManager *eM) {
	// Calculate the vector of the spring.
	auto		&springCompArr = eM->GetCompArr<Components::Spring>();
	const auto	&springs	   = springCompArr.Data();
	const size_t count		   = springs.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId	  = springCompArr.Index()[i];
		auto		  &currSpring = springs[i];
		if (entityId == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		} else if (currSpring.otherRigidBodyIndex == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Other RigidBody is missing!");
			continue;
		}

		const auto	   currRb			  = eM->GetTComponent<Components::RigidBody>(entityId);
		if (!currRb) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}
		if (!currRb->isAwake) continue;

		const auto	  &otherRb = eM->GetTComponent<Components::RigidBody>(currSpring.otherRigidBodyIndex);
		if (!eM->GetTComponent<Components::RigidBody>(currSpring.otherRigidBodyIndex)) {
			PE_LOG_WARN("Other RigidBody is missing!");
			continue;
		}


		// Calculate the vector of the spring.
		Math::RVec3 force = currRb->position - otherRb->position;

		// Calculate the magnitude of the force.
		// (Hooke's law: F = -k * Δx)
		const Math::real magnitude = currSpring.springConstant * (Math::Length(force) - currSpring.restLength);

		// Calculate the final force and apply to both sides.
		force = Math::Normalize(force) * -magnitude;
		currRb->forceAccum += force;
		otherRb->forceAccum -= force;
	}
}

void ForceGenerators::UpdateAeroForces(ECS::ECSManager *eM) {
	auto		&aeroCompArr = eM->GetCompArr<Components::Aero>();
	const auto	&aeros		 = aeroCompArr.Data();
	const size_t count		 = aeros.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId = aeroCompArr.Index()[i];
		auto		  &currAero = aeros[i];
		if (entityId == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}

		const auto	   currRb			  = eM->GetTComponent<Components::RigidBody>(entityId);
		if (!currRb) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}
		if (!currRb->isAwake) continue;

		// Calculate total velocity (wind speed and body’s velocity).
		const Math::RVec3 totalVelocity = currRb->velocity + windSpeed;

		// Calculate the velocity in local coordinates.
		const Math::RVec3 localVelocity = Math::Conjugate(currRb->orientation) * totalVelocity;

		// Calculate the force in local coordinates.
		const Math::RVec3 localForce = currAero.tensor * localVelocity;

		// Transform force back to world coordinates
		const Math::RVec3 worldForce = currRb->orientation * localForce;

		// Apply the force.
		Core::ForceApplicator::ApplyForceAtLocalPoint(*currRb, worldForce, currAero.position);
	}
}

void ForceGenerators::UpdateAeroControlForces(ECS::ECSManager *eM) {
	auto		&aeroControlCompArr = eM->GetCompArr<Components::AeroControl>();
	const auto	&aeroControls		= aeroControlCompArr.Data();
	const size_t count				= aeroControls.size();
	for (int i = 0; i < count; i++) {
		const uint32_t entityId		   = aeroControlCompArr.Index()[i];
		auto		  &currAeroControl = aeroControls[i];
		if (entityId == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}

		const auto	   currRb			  = eM->GetTComponent<Components::RigidBody>(entityId);
		if (!currRb) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}
		if (!currRb->isAwake) continue;

		// Mixes the 3 matrices according to the specified controlSetting value
		Math::RMat33 currentTensor;
		if (currAeroControl.controlSetting <= -1.0f)
			currentTensor = currAeroControl.minTensor;
		else if (currAeroControl.controlSetting >= 1.0f)
			currentTensor = currAeroControl.maxTensor;
		else if (currAeroControl.controlSetting < 0.0f) {
			currentTensor =
				Math::Mix(currAeroControl.minTensor, currAeroControl.baseTensor, currAeroControl.controlSetting + 1.0f);
		} else {
			currentTensor =
				Math::Mix(currAeroControl.baseTensor, currAeroControl.maxTensor, currAeroControl.controlSetting);
		}

		// Calculate total velocity (wind speed and body’s velocity).
		const Math::RVec3 totalVelocity = currRb->velocity + windSpeed;

		// Calculate the velocity in local coordinates.
		const Math::RVec3 localVelocity = Math::Conjugate(currRb->orientation) * totalVelocity;

		// Calculate the force in local coordinates.
		const Math::RVec3 localForce = currentTensor * localVelocity;

		// Transform force back to world coordinates
		const Math::RVec3 worldForce = currRb->orientation * localForce;

		// Apply the force.
		Core::ForceApplicator::ApplyForceAtLocalPoint(*currRb, worldForce, currAeroControl.position);
	}
}

void ForceGenerators::UpdateAngledAeroForces(ECS::ECSManager *eM, const Math::RVec3 &windSpeed) {
	auto &angledAeroArr = eM->GetCompArr<Components::AngledAero>();

	for (int i = 0; i < angledAeroArr.GetCount(); i++) {
		const uint32_t entityId = angledAeroArr.Index()[i];
		auto		  &currAero = angledAeroArr.Data()[i];
		if (entityId == ECS::INVALID_ENTITY_ID) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}

		const auto	   currRb			  = eM->GetTComponent<Components::RigidBody>(entityId);
		if (!currRb) {
			PE_LOG_WARN("Self RigidBody is missing!");
			continue;
		}
		if (!currRb->isAwake) continue;

		// Calculate total velocity (wind speed and body’s velocity).
		Math::RVec3 totalVelocity = currRb->velocity + windSpeed;

		// Calculate world orientation of surface
		Math::RQuat surfaceWorldOri = currRb->orientation * currAero.position;

		// Calculate the velocity in surface local coordinates
		Math::RVec3 surfaceLocalVel = Math::Conjugate(surfaceWorldOri) * totalVelocity;

		// Calculate the force in surface local coordinates
		Math::RVec3 surfaceLocalForce = currAero.tensor * surfaceLocalVel;

		// Transform force back to world coordinates
		Math::RVec3 worldForce = surfaceWorldOri * surfaceLocalForce;

		// Apply the force
		Core::ForceApplicator::ApplyForceAtLocalPoint(*currRb, worldForce, currAero.position);
	}
}
}  // namespace PE::Physics::Body