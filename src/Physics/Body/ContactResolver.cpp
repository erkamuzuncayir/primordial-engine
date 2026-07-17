#include "Physics/Body/ContactResolver.h"

#include <format>

#include "Physics/Body/Utilities.h"

namespace PE::Physics::Body {
// TODO: Put somewhere like physics config!
constexpr static Math::real VELOCITY_LIMIT				  = 0.25;
constexpr static uint32_t	POSITION_ITERATION_MULTIPLIER = 4;
constexpr static uint32_t	VELOCITY_ITERATION_MULTIPLIER = 4;
constexpr static Math::real POSITION_EPSILON			  = 0.01f;
constexpr static Math::real VELOCITY_EPSILON			  = 0.01f;

void ContactResolver::ResolveContacts(ECS::ECSManager *eM, std::vector<StaticContact> &staticContacts,
									  std::vector<KinematicContact> &kinematicContacts,
									  std::vector<RigidBodyContact> &rigidBodyContacts, const float dt) {
	if (staticContacts.empty() && kinematicContacts.empty() && rigidBodyContacts.empty()) return;

	PrepareSingleBodyContacts<StaticContact>(eM, staticContacts, dt);
	PrepareSingleBodyContacts<KinematicContact>(eM, kinematicContacts, dt);
	PrepareRigidBodyContacts(eM, rigidBodyContacts, dt);
	AdjustPositions(eM, staticContacts, kinematicContacts, rigidBodyContacts);
	AdjustVelocities(eM, staticContacts, kinematicContacts, rigidBodyContacts, dt);
}

template <typename SingleBodyContact>
void ContactResolver::PrepareSingleBodyContacts(ECS::ECSManager *eM, std::vector<SingleBodyContact> &contacts,
												const float dt) {
	auto &rbArr = eM->GetCompArr<Components::RigidBody>();
	for (SingleBodyContact &contact : contacts) {
		CalculateContactInternals(contact, &rbArr.Get(contact.entityOne), dt);
	}
}

void ContactResolver::PrepareRigidBodyContacts(ECS::ECSManager *eM, std::vector<RigidBodyContact> &contacts,
											   const float dt) {
	auto &rbArr = eM->GetCompArr<Components::RigidBody>();
	for (RigidBodyContact &contact : contacts) {
		CalculateContactInternals(contact, &rbArr.Get(contact.entityOne), &rbArr.Get(contact.entityTwo), dt);
	}
}

template <typename SingleBodyContact>
void ContactResolver::CalculateContactInternals(SingleBodyContact &contact, const Components::RigidBody *rbOne,
												const Math::real dt) {
	// Calculate a set of axes at the contact point.
	CalculateContactBasis(contact.contactToWorld, contact.normal);

	// Store the relative position of the contact relative to each body.
	contact.relativeContactPosition = contact.point - rbOne->position;

	// Find the relative velocity of the bodies at the contact point.
	contact.velocity = CalculateLocalVelocity(*rbOne, contact.relativeContactPosition, contact.contactToWorld, dt);
	if constexpr (std::is_same_v<SingleBodyContact, KinematicContact>) {
		Math::RVec3 kinVel = Math::Transpose(contact.contactToWorld) * contact.kinematicSurfaceVelocity;
		contact.velocity -= kinVel;
	}

	CalculateDesiredDeltaVelocity(contact, rbOne, dt);
}

void ContactResolver::CalculateContactInternals(RigidBodyContact &contact, const Components::RigidBody *rbOne,
												const Components::RigidBody *rbTwo, const Math::real dt) {
	// Calculate a set of axes at the contact point.
	CalculateContactBasis(contact.contactToWorld, contact.normal);

	// Store the relative position of the contact relative to each body.
	contact.relativeContactPosition[0] = contact.point - rbOne->position;
	contact.relativeContactPosition[1] = contact.point - rbTwo->position;

	// Find the relative velocity of the bodies at the contact point.
	contact.velocity = CalculateLocalVelocity(*rbOne, contact.relativeContactPosition[0], contact.contactToWorld, dt);
	contact.velocity -= CalculateLocalVelocity(*rbTwo, contact.relativeContactPosition[1], contact.contactToWorld, dt);

	CalculateDesiredDeltaVelocity(contact, rbOne, rbTwo, dt);
}

void ContactResolver::CalculateContactBasis(Math::RMat33 &outContactToWorld, const Math::RVec3 &normal) {
	Math::RVec3 tangent[2];

	// Check whether the Z-axis is nearer to the X or Y axis
	if (Math::Abs(normal.x) > Math::Abs(normal.y)) {
		// Scaling factor to ensure the results are normalized
		const Math::real s = 1.0f / Math::Sqrt(normal.z * normal.z + normal.x * normal.x);

		// The new X-axis is at right angles to the world Y-axis
		tangent[0].x = normal.z * s;
		tangent[0].y = 0.0f;
		tangent[0].z = -normal.x * s;
	} else {
		// The new X-axis is at right angles to the world X-axis
		const Math::real s = 1.0f / Math::Sqrt(normal.z * normal.z + normal.y * normal.y);

		tangent[0].x = 0.0f;
		tangent[0].y = -normal.z * s;
		tangent[0].z = normal.y * s;
	}
	tangent[1] = Math::Cross(normal, tangent[0]);

	outContactToWorld = {normal, tangent[0], tangent[1]};
}

Math::RVec3 ContactResolver::CalculateLocalVelocity(const Components::RigidBody &rb,
													const Math::RVec3			&relativeContactPos,
													const Math::RMat33 &contactToWorld, const Math::real dt) {
	// Work out the velocity of the contact point.
	const Math::RVec3 vel = Math::Cross(rb.angularVelocity, relativeContactPos) + rb.velocity;

	// Turn the velocity into contact-coordinates.
	Math::RVec3 contactVel = Math::Transpose(contactToWorld) * vel;

	// Calculate the amount of velocity that is due to forces without
	// reactions.
	Math::RVec3 accVel = rb.lastFrameAcceleration * dt;

	// Calculate the velocity in contact-coordinates.
	accVel = Math::Transpose(contactToWorld) * accVel;

	// We ignore any component of acceleration in the contact normal
	// direction, we are only interested in planar acceleration
	accVel.x = 0;

	// Add the planar velocities - if there's enough friction they will
	// be removed during velocity resolution
	contactVel += accVel;

	return contactVel;
}

template <typename SingleBodyContact>
void ContactResolver::CalculateDesiredDeltaVelocity(SingleBodyContact &contact, const Components::RigidBody *rbOne,
													Math::real dt) {
	Math::real velFromAcc{0};
	if (rbOne->isAwake) {
		// Calculate the acceleration-induced velocity accumulated in this frame.
		const Math::RVec3 scaledContact = dt * contact.normal;
		velFromAcc += Math::Dot(rbOne->lastFrameAcceleration, scaledContact);
	}

	// If the velocity is very slow, limit the restitution.
	Math::real thisRestitution = contact.restitution;
	if (Math::Abs(contact.velocity.x) < VELOCITY_LIMIT) thisRestitution = 0.0;

	// Combine the bounce velocity with the removed acceleration velocity.
	contact.desiredDeltaVelocity = -contact.velocity.x - thisRestitution * (contact.velocity.x - velFromAcc);
}

void ContactResolver::CalculateDesiredDeltaVelocity(RigidBodyContact &contact, const Components::RigidBody *rbOne,
													const Components::RigidBody *rbTwo, const Math::real dt) {
	const Math::RVec3 scaledContact = dt * contact.normal;
	Math::real		  velFromAcc{0};
	if (rbOne->isAwake) {
		// Calculate the acceleration-induced velocity accumulated in this frame.
		velFromAcc += Math::Dot(rbOne->lastFrameAcceleration, scaledContact);
	}

	if (rbTwo->isAwake) velFromAcc -= Math::Dot(rbTwo->lastFrameAcceleration, scaledContact);

	// If the velocity is very slow, limit the restitution.
	Math::real thisRestitution = contact.restitution;
	if (Math::Abs(contact.velocity.x) < VELOCITY_LIMIT) thisRestitution = 0.0;

	// Combine the bounce velocity with the removed acceleration velocity.
	contact.desiredDeltaVelocity = -contact.velocity.x - thisRestitution * (contact.velocity.x - velFromAcc);
}

void ContactResolver::AdjustPositions(ECS::ECSManager *eM, std::vector<StaticContact> &staticContacts,
									  std::vector<KinematicContact> &kinematicContacts,
									  std::vector<RigidBodyContact> &rigidBodyContacts) {
	const size_t iterationCount =
		(staticContacts.size() + kinematicContacts.size() + rigidBodyContacts.size()) * POSITION_ITERATION_MULTIPLIER;
	if (iterationCount == 0) return;

	size_t iterationsUsed = 0;
	while (iterationsUsed < iterationCount) {
		Math::real maxPenetration = POSITION_EPSILON;

		enum class ContactType { NONE, STATIC, KINEMATIC, RIGID } bestType = ContactType::NONE;
		size_t bestIndex												   = 0;

		for (size_t i = 0; i < staticContacts.size(); i++) {
			if (staticContacts[i].penetration > maxPenetration) {
				maxPenetration = staticContacts[i].penetration;
				bestIndex	   = i;
				bestType	   = ContactType::STATIC;
			}
		}
		for (size_t i = 0; i < kinematicContacts.size(); i++) {
			if (kinematicContacts[i].penetration > maxPenetration) {
				maxPenetration = kinematicContacts[i].penetration;
				bestIndex	   = i;
				bestType	   = ContactType::KINEMATIC;
			}
		}
		for (size_t i = 0; i < rigidBodyContacts.size(); i++) {
			if (rigidBodyContacts[i].penetration > maxPenetration) {
				maxPenetration = rigidBodyContacts[i].penetration;
				bestIndex	   = i;
				bestType	   = ContactType::RIGID;
			}
		}

		if (bestType == ContactType::NONE) break;

		Math::RVec3	  linearChange[2]{Math::RVec3Zero, Math::RVec3Zero};
		Math::RVec3	  angularChange[2]{Math::RVec3Zero, Math::RVec3Zero};
		ECS::EntityID movedEntities[2] = {ECS::INVALID_ENTITY_ID, ECS::INVALID_ENTITY_ID};

		if (bestType == ContactType::STATIC) {
			auto const &c  = staticContacts[bestIndex];
			Components::RigidBody &rb = *eM->GetTComponent<Components::RigidBody>(c.entityOne);
			ApplyPositionChange(rb, c, linearChange[0], angularChange[0], maxPenetration);
			movedEntities[0] = c.entityOne;
		} else if (bestType == ContactType::KINEMATIC) {
			auto const				  &c  = kinematicContacts[bestIndex];
			Components::RigidBody &rb = *eM->GetTComponent<Components::RigidBody>(c.entityOne);
			if (!rb.isAwake) rb.WakeUp(c.desiredDeltaVelocity);
			ApplyPositionChange(rb, c, linearChange[0], angularChange[0], maxPenetration);
			movedEntities[0] = c.entityOne;
		} else {
			auto const &c	 = rigidBodyContacts[bestIndex];
			Components::RigidBody &rbOne = *eM->GetTComponent<Components::RigidBody>(c.entityOne);
			Components::RigidBody &rbTwo = *eM->GetTComponent<Components::RigidBody>(c.entityTwo);
			if (rbOne.isAwake ^ rbTwo.isAwake) {
				if (rbOne.isAwake)
					rbTwo.WakeUp(c.desiredDeltaVelocity);
				else
					rbOne.WakeUp(c.desiredDeltaVelocity);
			}
			ApplyPositionChange(rbOne, rbTwo, c, linearChange, angularChange, maxPenetration);
			movedEntities[0] = c.entityOne;
			movedEntities[1] = c.entityTwo;
		}
		UpdatePositionsOfOtherContacts(staticContacts, kinematicContacts, rigidBodyContacts, movedEntities,
									   linearChange, angularChange);
		iterationsUsed++;
	}
}

template <typename SingleBodyContact>
void ContactResolver::ApplyPositionChange(Components::RigidBody &rb, const SingleBodyContact &contact,
										  Math::RVec3 &positionChange, Math::RVec3 &orientationChange,
										  const Math::real &penetration) {
	Math::real angularMove{0.0};
	Math::real linearMove{0.0};
	Math::real totalInertia{0.0};
	Math::real linearInertia{0.0};
	Math::real angularInertia{0.0};

	// We need to work out the inertia of the object in the direction of the contact normal, due to angular inertia
	// only.
	Math::RMat33 invInertia = rb.inverseInertiaTensorWorld;

	// Use the same procedure as for calculating frictionless velocity change to work out the angular inertia.
	Math::RVec3 angularInertiaWorld = Math::Cross(contact.relativeContactPosition, contact.normal);
	angularInertiaWorld				= invInertia * angularInertiaWorld;
	angularInertiaWorld				= Math::Cross(angularInertiaWorld, contact.relativeContactPosition);
	angularInertia					= Math::Dot(angularInertiaWorld, contact.normal);

	// The linear component is simply the inverse mass
	linearInertia = rb.inverseMass;

	// Keep track of the total inertia from all components
	totalInertia += linearInertia + angularInertia;

	constexpr Math::real angularLimit = 0.2f;
	// The linear and angular movements required are in proportion to the two inverse inertias.
	angularMove = penetration * (angularInertia / totalInertia);
	linearMove	= penetration * (linearInertia / totalInertia);

	// To avoid angular projections that are too great (when mass is large
	// but inertia tensor is small) limit the angular move.
	Math::RVec3 projection =
		contact.relativeContactPosition - contact.normal * Math::Dot(contact.relativeContactPosition, contact.normal);

	// Use the small angle approximation for the sine of the angle (i.e. the magnitude would be
	// sine(angularLimit) * projection.magnitude but we approximate sine(angularLimit) to angularLimit).
	Math::real maxMagnitude = angularLimit * Math::Length(projection);

	if (angularMove < -maxMagnitude) {
		Math::real totalMove = angularMove + linearMove;
		angularMove			 = -maxMagnitude;
		linearMove			 = totalMove - angularMove;
	} else if (angularMove > maxMagnitude) {
		Math::real totalMove = angularMove + linearMove;
		angularMove			 = maxMagnitude;
		linearMove			 = totalMove - angularMove;
	}

	// We have the linear amount of movement required by turning the rigid body (in angularMove).
	// We now need to calculate the desired rotation to achieve that.
	if (angularMove == 0.0f) {
		// Easy case - no angular movement means no rotation.
		orientationChange = Math::RVec3Zero;
	} else {
		// Work out the direction we'd like to rotate in.
		Math::RVec3 targetAngularDirection = Math::Cross(contact.relativeContactPosition, contact.normal);

		// Work out the direction we'd need to rotate to achieve that
		orientationChange = (rb.inverseInertiaTensorWorld * targetAngularDirection) * (angularMove / angularInertia);
	}

	// Velocity change is easier - it is just the linear movement along the contact normal.
	positionChange = contact.normal * linearMove;

	// Now we can start to apply the values we've calculated. Apply the linear movement
	rb.position += positionChange;
	// And the change in orientation
	Math::RQuat q = rb.orientation;
	Math::RQuat w(0.0f, orientationChange.x, orientationChange.y, orientationChange.z);
	Math::RQuat qDelta = w * q;

	q.w += qDelta.w * 0.5f;
	q.x += qDelta.x * 0.5f;
	q.y += qDelta.y * 0.5f;
	q.z += qDelta.z * 0.5f;
	q = Math::Normalize(q);

	rb.orientation = q;
	rb.isDirty	   = true;

	// We need to calculate the derived data for any body that is asleep, so that the changes are reflected
	// in the object's data. Otherwise the resolution will not change the position of the object,
	// and the next collision detection round will have the same penetration.
	if (!rb.isAwake) rb.UpdateDerivedData();
}

void ContactResolver::ApplyPositionChange(Components::RigidBody &rbOne, Components::RigidBody &rbTwo,
										  const RigidBodyContact &contact, Math::RVec3 positionChange[2],
										  Math::RVec3 orientationChange[2], const Math::real penetration) {
	Math::real angularMove[2] = {0.0f, 0.0f};
	Math::real linearMove[2]  = {0.0f, 0.0f};

	Math::real totalInertia		 = 0.0f;
	Math::real linearInertia[2]	 = {0.0f, 0.0f};
	Math::real angularInertia[2] = {0.0f, 0.0f};

	Components::RigidBody *rb[2] = {&rbOne, &rbTwo};

	// We need to work out the inertia of each object in the direction of the contact normal, due to angular inertia
	// only.
	for (uint32_t i = 0; i < 2; i++) {
		if (rb[i]) {
			Math::RMat33 invInertia = rb[i]->inverseInertiaTensorWorld;

			// Use the same procedure as for calculating frictionless velocity change to work out the angular inertia.
			Math::RVec3 angularInertiaWorld = Math::Cross(contact.relativeContactPosition[i], contact.normal);
			angularInertiaWorld				= invInertia * angularInertiaWorld;
			angularInertiaWorld				= Math::Cross(angularInertiaWorld, contact.relativeContactPosition[i]);
			angularInertia[i]				= Math::Dot(angularInertiaWorld, contact.normal);

			// The linear component is simply the inverse mass
			linearInertia[i] = rb[i]->inverseMass;

			// Keep track of the total inertia from all components
			totalInertia += linearInertia[i] + angularInertia[i];

			// We break the loop here so that the totalInertia value is completely calculated
			// (by both iterations) before continuing.
		}
	}

	// Loop through again calculating and applying the changes
	for (uint32_t i = 0; i < 2; i++) {
		if (rb[i]) {
			constexpr Math::real angularLimit = 0.2f;
			// The linear and angular movements required are in proportion to the two inverse inertias.
			Math::real sign = (i == 0) ? 1.0f : -1.0f;
			angularMove[i]	= sign * penetration * (angularInertia[i] / totalInertia);
			linearMove[i]	= sign * penetration * (linearInertia[i] / totalInertia);

			// To avoid angular projections that are too great (when mass is large
			// but inertia tensor is small) limit the angular move.
			Math::RVec3 projection = contact.relativeContactPosition[i] -
									 contact.normal * Math::Dot(contact.relativeContactPosition[i], contact.normal);

			// Use the small angle approximation for the sine of the angle (i.e. the magnitude would be
			// sine(angularLimit) * projection.magnitude but we approximate sine(angularLimit) to angularLimit).
			Math::real maxMagnitude = angularLimit * Math::Length(projection);

			if (angularMove[i] < -maxMagnitude) {
				Math::real totalMove = angularMove[i] + linearMove[i];
				angularMove[i]		 = -maxMagnitude;
				linearMove[i]		 = totalMove - angularMove[i];
			} else if (angularMove[i] > maxMagnitude) {
				Math::real totalMove = angularMove[i] + linearMove[i];
				angularMove[i]		 = maxMagnitude;
				linearMove[i]		 = totalMove - angularMove[i];
			}

			// We have the linear amount of movement required by turning the rigid body (in angularMove[i]).
			// We now need to calculate the desired rotation to achieve that.
			if (angularMove[i] == 0.0f) {
				// Easy case - no angular movement means no rotation.
				orientationChange[i] = Math::RVec3Zero;
			} else {
				// Work out the direction we'd like to rotate in.
				Math::RVec3 targetAngularDirection = Math::Cross(contact.relativeContactPosition[i], contact.normal);

				// Work out the direction we'd need to rotate to achieve that
				orientationChange[i] =
					(rb[i]->inverseInertiaTensorWorld * targetAngularDirection) * (angularMove[i] / angularInertia[i]);
			}

			// Velocity change is easier - it is just the linear movement along the contact normal.
			positionChange[i] = contact.normal * linearMove[i];

			// Now we can start to apply the values we've calculated. Apply the linear movement
			rb[i]->position += positionChange[i];

			// And the change in orientation
			Math::RQuat q = rb[i]->orientation;
			Math::RQuat w(0.0f, orientationChange[i].x, orientationChange[i].y, orientationChange[i].z);
			Math::RQuat qDelta = w * q;

			q.w += qDelta.w * 0.5f;
			q.x += qDelta.x * 0.5f;
			q.y += qDelta.y * 0.5f;
			q.z += qDelta.z * 0.5f;

			q				   = Math::Normalize(q);
			rb[i]->orientation = q;

			// We need to calculate the derived data for any body that is asleep, so that the changes are reflected
			// in the object's data. Otherwise the resolution will not change the position of the object,
			// and the next collision detection round will have the same penetration.
			if (!rb[i]->isAwake) rb[i]->UpdateDerivedData();
		}
	}
}

void ContactResolver::UpdatePositionsOfOtherContacts(std::vector<StaticContact>	   &staticContacts,
													 std::vector<KinematicContact> &kinematicContacts,
													 std::vector<RigidBodyContact> &rigidBodyContacts,
													 const ECS::EntityID moved[2], const Math::RVec3 linearChange[2],
													 const Math::RVec3 angularChange[2]) {
	const ECS::EntityID m0 = moved[0];
	const ECS::EntityID m1 = moved[1];

	auto updateSingleBody = [&](auto &contacts) {
		for (auto &c : contacts) {
			if (c.entityOne == m0) {
				Math::RVec3 delta = linearChange[0] + Math::Cross(angularChange[0], c.relativeContactPosition);
				c.penetration -= Math::Dot(delta, c.normal);
			} else if (c.entityOne == m1) {
				Math::RVec3 delta = linearChange[1] + Math::Cross(angularChange[1], c.relativeContactPosition);
				c.penetration -= Math::Dot(delta, c.normal);
			}
		}
	};

	updateSingleBody(staticContacts);
	updateSingleBody(kinematicContacts);

	for (auto &c : rigidBodyContacts) {
		if (c.entityOne == m0) {
			Math::RVec3 deltaPosition = linearChange[0] + Math::Cross(angularChange[0], c.relativeContactPosition[0]);
			c.penetration -= Math::Dot(deltaPosition, c.normal);
		} else if (c.entityTwo == m0) {
			Math::RVec3 deltaPosition = linearChange[0] + Math::Cross(angularChange[0], c.relativeContactPosition[1]);
			c.penetration += Math::Dot(deltaPosition, c.normal);
		}

		if (c.entityOne == m1) {
			Math::RVec3 deltaPosition = linearChange[1] + Math::Cross(angularChange[1], c.relativeContactPosition[0]);
			c.penetration -= Math::Dot(deltaPosition, c.normal);
		} else if (c.entityTwo == m1) {
			Math::RVec3 deltaPosition = linearChange[1] + Math::Cross(angularChange[1], c.relativeContactPosition[1]);
			c.penetration += Math::Dot(deltaPosition, c.normal);
		}
	}
}

void ContactResolver::AdjustVelocities(ECS::ECSManager *eM, std::vector<StaticContact> &staticContacts,
									   std::vector<KinematicContact> &kinematicContacts,
									   std::vector<RigidBodyContact> &rigidBodyContacts, const Math::real dt) {
	const size_t iterationCount =
		(staticContacts.size() + kinematicContacts.size() + rigidBodyContacts.size()) * VELOCITY_ITERATION_MULTIPLIER;
	if (iterationCount == 0) return;

	size_t iterationsUsed = 0;
	while (iterationsUsed < iterationCount) {
		Math::real maxVelocity = VELOCITY_EPSILON;

		enum class ContactType { NONE, STATIC, KINEMATIC, RIGID };
		ContactType bestType = ContactType::NONE;

		size_t bestIndex												   = 0;

		for (size_t i = 0; i < staticContacts.size(); i++) {
			if (staticContacts[i].desiredDeltaVelocity > maxVelocity) {
				maxVelocity = staticContacts[i].desiredDeltaVelocity;
				bestIndex	= i;
				bestType	= ContactType::STATIC;
			}
		}
		for (size_t i = 0; i < kinematicContacts.size(); i++) {
			if (kinematicContacts[i].desiredDeltaVelocity > maxVelocity) {
				maxVelocity = kinematicContacts[i].desiredDeltaVelocity;
				bestIndex	= i;
				bestType	= ContactType::KINEMATIC;
			}
		}
		for (size_t i = 0; i < rigidBodyContacts.size(); i++) {
			if (rigidBodyContacts[i].desiredDeltaVelocity > maxVelocity) {
				maxVelocity = rigidBodyContacts[i].desiredDeltaVelocity;
				bestIndex	= i;
				bestType	= ContactType::RIGID;
			}
		}

		if (bestType == ContactType::NONE) break;

		Math::RVec3	  velocityChange[2]{Math::RVec3Zero, Math::RVec3Zero};
		Math::RVec3	  angularVelocityChange[2]{Math::RVec3Zero, Math::RVec3Zero};
		ECS::EntityID movedEntities[2] = {ECS::INVALID_ENTITY_ID, ECS::INVALID_ENTITY_ID};

		if (bestType == ContactType::STATIC) {
			auto const &c  = staticContacts[bestIndex];
			Components::RigidBody &rb = *eM->GetTComponent<Components::RigidBody>(c.entityOne);
			ApplyVelocityChange(rb, c, velocityChange[0], angularVelocityChange[0]);
			movedEntities[0] = c.entityOne;
		} else if (bestType == ContactType::KINEMATIC) {
			auto const &c  = kinematicContacts[bestIndex];
			Components::RigidBody &rb = *eM->GetTComponent<Components::RigidBody>(c.entityOne);
			if (!rb.isAwake) rb.WakeUp(c.desiredDeltaVelocity);
			ApplyVelocityChange(rb, c, velocityChange[0], angularVelocityChange[0]);
			movedEntities[0] = c.entityOne;
		} else {
			auto const &c	 = rigidBodyContacts[bestIndex];
			Components::RigidBody &rbOne = *eM->GetTComponent<Components::RigidBody>(c.entityOne);
			Components::RigidBody &rbTwo = *eM->GetTComponent<Components::RigidBody>(c.entityTwo);
			if (rbOne.isAwake ^ rbTwo.isAwake) {
				if (rbOne.isAwake)
					rbTwo.WakeUp(c.desiredDeltaVelocity);
				else
					rbOne.WakeUp(c.desiredDeltaVelocity);
			}
			ApplyVelocityChange(rbOne, rbTwo, c, velocityChange, angularVelocityChange);
			movedEntities[0] = c.entityOne;
			movedEntities[1] = c.entityTwo;
		}
		UpdateVelocitiesOfOtherContacts(eM, staticContacts, kinematicContacts, rigidBodyContacts, movedEntities,
										velocityChange, angularVelocityChange, dt);
		iterationsUsed++;
	}
}

template <typename SingleBodyContact>
void ContactResolver::ApplyVelocityChange(Components::RigidBody &rb, const SingleBodyContact &contact,
										  Math::RVec3 &velocityChange, Math::RVec3 &angularVelocityChange) {
	// We will calculate the impulse for each contact axis
	Math::RVec3 impulseContact;
	if (contact.staticFriction < Math::REpsilon && contact.dynamicFriction < Math::REpsilon) {
		// Use the short format for frictionless contacts
		impulseContact = CalculateFrictionlessImpulse(rb, contact);
	} else {
		// Otherwise we may have impulses that aren't in the direction of the contact, so we need the more complex
		// version.
		impulseContact = CalculateFrictionImpulse(rb, contact);
	}

	// Convert impulse to world coordinates
	const Math::RVec3 impulseWorld = contact.contactToWorld * impulseContact;

	// Split in the impulse into linear and angular components
	// Torque = R x F (Distance x Force)
	const Math::RVec3 impulsiveTorque = Math::Cross(contact.relativeContactPosition, impulseWorld);
	// Angular velocity change = Inverse Inertia Matrix * Torque
	angularVelocityChange = rb.inverseInertiaTensorWorld * impulsiveTorque;
	// Linear velocity change = Thrust * Inverse Mass (F = m*a logic)
	velocityChange = impulseWorld * rb.inverseMass;

	// Apply the changes
	rb.velocity += velocityChange;
	rb.angularVelocity += angularVelocityChange;
}

void ContactResolver::ApplyVelocityChange(Components::RigidBody &rbOne, Components::RigidBody &rbTwo,
										  const RigidBodyContact &contact, Math::RVec3 velocityChange[2],
										  Math::RVec3 angularVelocityChange[2]) {
	// We will calculate the impulse for each contact axis
	Math::RVec3 impulseContact;

	if (contact.staticFriction < Math::REpsilon && contact.dynamicFriction < Math::REpsilon) {
		// Use the short format for frictionless contacts
		impulseContact = CalculateFrictionlessImpulse(rbOne, rbTwo, contact);
	} else {
		// Otherwise we may have impulses that aren't in the direction of the contact, so we need the more complex
		// version.
		impulseContact = CalculateFrictionImpulse(rbOne, rbTwo, contact);
	}

	// Convert impulse to world coordinates
	const Math::RVec3 impulseWorld = contact.contactToWorld * impulseContact;

	// Split in the impulse into linear and angular components
	// Torque = R x F (Distance x Force)
	Math::RVec3 impulsiveTorque = Math::Cross(contact.relativeContactPosition[0], impulseWorld);
	// Angular velocity change = Inverse Inertia Matrix * Torque
	angularVelocityChange[0] = rbOne.inverseInertiaTensorWorld * impulsiveTorque;
	// Linear velocity change = Thrust * Inverse Mass (F = m*a logic)
	velocityChange[0] = impulseWorld * rbOne.inverseMass;

	// Apply the changes
	rbOne.velocity += velocityChange[0];
	rbOne.angularVelocity += angularVelocityChange[0];

	// The second object is pushed in the opposite direction of the action
	impulsiveTorque = Math::Cross(impulseWorld, contact.relativeContactPosition[1]);

	// Reverse order
	angularVelocityChange[1] = rbTwo.inverseInertiaTensorWorld * impulsiveTorque;
	velocityChange[1]		 = impulseWorld * -rbTwo.inverseMass;  // Reverse direction

	rbTwo.velocity += velocityChange[1];
	rbTwo.angularVelocity += angularVelocityChange[1];
}

void ContactResolver::UpdateVelocitiesOfOtherContacts(ECS::ECSManager *eM, std::vector<StaticContact> &staticContacts,
													  std::vector<KinematicContact> &kinematicContacts,
													  std::vector<RigidBodyContact> &rigidBodyContacts,
													  const ECS::EntityID moved[2], const Math::RVec3 velocityChange[2],
													  const Math::RVec3 angularVelocityChange[2], const Math::real dt) {
	auto			   &rbArr = eM->GetCompArr<Components::RigidBody>();
	const ECS::EntityID m0	  = moved[0];
	const ECS::EntityID m1	  = moved[1];

	auto updateSingleBody = [&](auto &contacts) {
		for (auto &c : contacts) {
			if (c.entityOne == m0) {
				Math::RVec3 deltaVel =
					velocityChange[0] + Math::Cross(angularVelocityChange[0], c.relativeContactPosition);
				c.velocity += (Math::Transpose(c.contactToWorld) * deltaVel);
				CalculateDesiredDeltaVelocity(c, &rbArr.Get(c.entityOne), dt);
			} else if (c.entityOne == m1) {
				Math::RVec3 deltaVel =
					velocityChange[1] + Math::Cross(angularVelocityChange[1], c.relativeContactPosition);
				c.velocity += (Math::Transpose(c.contactToWorld) * deltaVel);
				CalculateDesiredDeltaVelocity(c, &rbArr.Get(c.entityOne), dt);
			}
		}
	};

	updateSingleBody(staticContacts);
	updateSingleBody(kinematicContacts);

	for (auto &c : rigidBodyContacts) {
		bool updated = false;

		if (c.entityOne == m0) {
			Math::RVec3 deltaVel =
				velocityChange[0] + Math::Cross(angularVelocityChange[0], c.relativeContactPosition[0]);
			c.velocity += (Math::Transpose(c.contactToWorld) * deltaVel);
			updated = true;
		} else if (c.entityTwo == m0) {
			Math::RVec3 deltaVel =
				velocityChange[0] + Math::Cross(angularVelocityChange[0], c.relativeContactPosition[1]);
			c.velocity -= (Math::Transpose(c.contactToWorld) * deltaVel);
			updated = true;
		}

		if (c.entityOne == m1) {
			Math::RVec3 deltaVel =
				velocityChange[1] + Math::Cross(angularVelocityChange[1], c.relativeContactPosition[0]);
			c.velocity += (Math::Transpose(c.contactToWorld) * deltaVel);
			updated = true;
		} else if (c.entityTwo == m1) {
			Math::RVec3 deltaVel =
				velocityChange[1] + Math::Cross(angularVelocityChange[1], c.relativeContactPosition[1]);
			c.velocity -= (Math::Transpose(c.contactToWorld) * deltaVel);
			updated = true;
		}

		if (updated) CalculateDesiredDeltaVelocity(c, &rbArr.Get(c.entityOne), &rbArr.Get(c.entityTwo), dt);
	}
}

template <typename SingleBodyContact>
Math::RVec3 ContactResolver::CalculateFrictionlessImpulse(const Components::RigidBody &rb,
														  const SingleBodyContact	  &contact) {
	// Build a vector that shows the change in velocity in world space for a unit impulse in the direction of the
	// contact normal.
	Math::RVec3 deltaVelWorld = Math::Cross(contact.relativeContactPosition, contact.normal);
	deltaVelWorld			  = rb.inverseInertiaTensorWorld * deltaVelWorld;
	deltaVelWorld			  = Math::Cross(deltaVelWorld, contact.relativeContactPosition);

	// Work out the change in velocity in contact coordinates.
	Math::real deltaVelocity = Math::Dot(deltaVelWorld, contact.normal);

	// Add the linear component of velocity change
	deltaVelocity += rb.inverseMass;

	// Calculate the required size of the impulse
	Math::RVec3 impulseContact(0.0f);
	impulseContact.x = contact.desiredDeltaVelocity / deltaVelocity;
	impulseContact.y = 0.0f;
	impulseContact.z = 0.0f;

	return impulseContact;
}

Math::RVec3 ContactResolver::CalculateFrictionlessImpulse(const Components::RigidBody &rbOne,
														  const Components::RigidBody &rbTwo,
														  const RigidBodyContact	  &contact) {
	Math::real					 deltaVelocity{0.0};
	constexpr uint8_t			 size	  = 2;
	const Components::RigidBody *rb[size] = {&rbOne, &rbTwo};
	for (size_t i = 0; i < size; ++i) {
		// Build a vector that shows the change in velocity in world space for a unit impulse in the direction of the
		// contact normal.
		Math::RVec3 deltaVelWorld = Math::Cross(contact.relativeContactPosition[i], contact.normal);
		deltaVelWorld			  = rb[i]->inverseInertiaTensorWorld * deltaVelWorld;
		deltaVelWorld			  = Math::Cross(deltaVelWorld, contact.relativeContactPosition[i]);

		// Work out the change in velocity in contact coordinates.
		deltaVelocity += Math::Dot(deltaVelWorld, contact.normal);

		// Add the linear component of velocity change
		deltaVelocity += rb[i]->inverseMass;
	}

	// Calculate the required size of the impulse
	Math::RVec3 impulseContact(0.0);
	impulseContact.x = contact.desiredDeltaVelocity / deltaVelocity;
	impulseContact.y = 0.0f;
	impulseContact.z = 0.0f;

	return impulseContact;
}

template <typename SingleBodyContact>
Math::RVec3 ContactResolver::CalculateFrictionImpulse(const Components::RigidBody &rb,
													  const SingleBodyContact	  &contact) {
	const Math::real inverseMass = rb.inverseMass;

	// The equivalent of a cross product in matrices is multiplication by a skew symmetric matrix - we build the matrix
	// for converting between linear and angular quantities.
	const Math::RMat33 impulseToTorque = Math::SkewSymmetric(contact.relativeContactPosition);

	// Build the matrix to convert contact impulse to change in velocity in world coordinates.
	Math::RMat33 deltaVelWorld = impulseToTorque;
	deltaVelWorld *= rb.inverseInertiaTensorWorld;
	deltaVelWorld *= impulseToTorque;
	deltaVelWorld *= -1.0f;

	// Do a change of basis to convert into contact coordinates.
	Math::RMat33 deltaVelocity = Math::Transpose(contact.contactToWorld) * deltaVelWorld * contact.contactToWorld;

	// Add in the linear velocity change to the diagonal
	deltaVelocity[0][0] += inverseMass;
	deltaVelocity[1][1] += inverseMass;
	deltaVelocity[2][2] += inverseMass;

	// Invert to get the impulse needed per unit velocity
	const Math::RMat33 impulseMatrix = Math::Inverse(deltaVelocity);

	// Find the target velocities to kill
	const Math::RVec3 velKill(contact.desiredDeltaVelocity, -contact.velocity.y, -contact.velocity.z);

	// Find the impulse to kill target velocities
	Math::RVec3 impulseContact = impulseMatrix * velKill;

	// Check for exceeding friction
	const Math::real planarImpulse =
		Math::Sqrt(impulseContact.y * impulseContact.y + impulseContact.z * impulseContact.z);

	if (planarImpulse > impulseContact.x * contact.staticFriction) {
		// We need to use dynamic friction
		impulseContact.y /= planarImpulse;
		impulseContact.z /= planarImpulse;

		impulseContact.x = deltaVelocity[0][0] + deltaVelocity[1][0] * contact.dynamicFriction * impulseContact.y +
						   deltaVelocity[2][0] * contact.dynamicFriction * impulseContact.z;

		impulseContact.x = contact.desiredDeltaVelocity / impulseContact.x;
		impulseContact.y *= contact.dynamicFriction * impulseContact.x;
		impulseContact.z *= contact.dynamicFriction * impulseContact.x;
	}

	return impulseContact;
}

Math::RVec3 ContactResolver::CalculateFrictionImpulse(const Components::RigidBody &rbOne,
													  const Components::RigidBody &rbTwo,
													  const RigidBodyContact	  &contact) {
	constexpr uint8_t			 size	  = 2;
	const Components::RigidBody *rb[size] = {&rbOne, &rbTwo};

	Math::RMat33 totalDeltaVelWorld{0};
	Math::real	 inverseMass{0};
	for (size_t i = 0; i < size; ++i) {
		// The equivalent of a cross product in matrices is multiplication by a skew symmetric matrix - we build the
		// matrix for converting between linear and angular quantities.
		const Math::RMat33 impulseToTorque = Math::SkewSymmetric(contact.relativeContactPosition[i]);

		// Build the matrix to convert contact impulse to change in velocity in world coordinates.
		Math::RMat33 deltaVelWorld = impulseToTorque;
		deltaVelWorld *= rb[i]->inverseInertiaTensorWorld;
		deltaVelWorld *= impulseToTorque;
		deltaVelWorld *= -1.0f;

		// Add to the total delta velocity
		totalDeltaVelWorld += deltaVelWorld;

		// Add to the inverse mass
		inverseMass += rb[i]->inverseMass;
	}

	// Do a change of basis to convert into contact coordinates.
	Math::RMat33 deltaVelocity = Math::Transpose(contact.contactToWorld) * totalDeltaVelWorld * contact.contactToWorld;

	// Add in the linear velocity change to the diagonal
	deltaVelocity[0][0] += inverseMass;
	deltaVelocity[1][1] += inverseMass;
	deltaVelocity[2][2] += inverseMass;

	// Invert to get the impulse needed per unit velocity
	const Math::RMat33 impulseMatrix = Math::Inverse(deltaVelocity);

	// Find the target velocities to kill
	const Math::RVec3 velKill(contact.desiredDeltaVelocity, -contact.velocity.y, -contact.velocity.z);

	// Find the impulse to kill target velocities
	Math::RVec3 impulseContact = impulseMatrix * velKill;

	// Check for exceeding friction
	const Math::real planarImpulse =
		Math::Sqrt(impulseContact.y * impulseContact.y + impulseContact.z * impulseContact.z);

	if (planarImpulse > impulseContact.x * contact.staticFriction) {
		// We need to use dynamic friction
		impulseContact.y /= planarImpulse;
		impulseContact.z /= planarImpulse;

		impulseContact.x = deltaVelocity[0][0] + deltaVelocity[1][0] * contact.dynamicFriction * impulseContact.y +
						   deltaVelocity[2][0] * contact.dynamicFriction * impulseContact.z;

		impulseContact.x = contact.desiredDeltaVelocity / impulseContact.x;
		impulseContact.y *= contact.dynamicFriction * impulseContact.x;
		impulseContact.z *= contact.dynamicFriction * impulseContact.x;
	}

	return impulseContact;
}
}  // namespace PE::Physics::Body