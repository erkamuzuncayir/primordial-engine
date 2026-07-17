#include "Physics/Body/Components/RigidBody.h"

#include <format>

#include "Utilities/Logger.h"

namespace PE::Physics::Body::Components {
void RigidBody::UpdateDerivedData() {
	orientation = Math::Normalize(orientation);
	CalculateInverseInertiaTensorWorld();
	isDirty = true;
}

void RigidBody::CalculateInverseInertiaTensorWorld() {
	// TODO: If remove position and orientation from rigidbody, Might be get localmatrix from transform?
	const Math::RMat33 rotationMatrix = Math::QuatToRMat33(orientation);
	inverseInertiaTensorWorld		  = rotationMatrix * inverseInertiaTensor * Math::Transpose(rotationMatrix);
}

void RigidBody::ClearAccumulators() {
	forceAccum	= Math::RVec3(0.0f);
	torqueAccum = Math::RVec3(0.0f);
}

void RigidBody::Sleep() {
	isAwake			= false;
	velocity		= Math::RVec3Zero;
	angularVelocity = Math::RVec3Zero;
	motion			= 0.0f;
	ClearAccumulators();
}

void RigidBody::WakeUp(const Math::real newMotion) {
	isAwake = true;
	// motion	= Math::Min(5.0 * Math::Abs(newMotion), 2.0 * SLEEP_EPSILON);
	motion	= 1.0 * Math::Abs(newMotion);
}

void RigidBody::SetMass(const Math::real mass) {
	const Math::real oldInverseMass = inverseMass;
	inverseMass						= mass <= 0.0 ? 0.0 : 1 / mass;

	UpdateInertiaTensor(oldInverseMass, inverseMass);
}

void RigidBody::UpdateInertiaTensor(const Math::real oldInverseMass, const Math::real newInverseMass) {
	if (newInverseMass <= 0.0)
		inverseInertiaTensor = Math::RMat33{0.0};
	else {
		if (oldInverseMass <= 0.0)
			PE_LOG_WARN("Cannot implicitly change mass of a static body. Use SetMassAndInertia methods!");
		else {
			const Math::real massRatio = newInverseMass / oldInverseMass;
			inverseInertiaTensor *= massRatio;
		}
	}
	UpdateDerivedData();
}

void RigidBody::SetInertiaTensorForBoxCollider(const Math::RVec3 &extents) {
	if (inverseMass <= 0.0f) {
		inverseInertiaTensor = Math::RMat33(0.0f);
	} else {
		const Math::real mass = 1.0f / inverseMass;
		Math::RMat33	 inertia(0.0f);

		// I = 1/3 * m * (half_a^2 + half_b^2)
		inertia[0][0] = (1.0f / 3.0f) * mass * (extents.y * extents.y + extents.z * extents.z);
		inertia[1][1] = (1.0f / 3.0f) * mass * (extents.x * extents.x + extents.z * extents.z);
		inertia[2][2] = (1.0f / 3.0f) * mass * (extents.x * extents.x + extents.y * extents.y);

		inverseInertiaTensor = Math::Inverse(inertia);
	}
	UpdateDerivedData();
}

void RigidBody::SetInertiaTensorForSphereCollider(const Math::real radius) {
	if (inverseMass <= 0.0f) {
		inverseInertiaTensor = Math::RMat33(0.0f);
	} else {
		const Math::real mass = 1 / inverseMass;
		Math::RMat33	 inertia(0.0f);

		// I = 2/5 * m * r^2
		const Math::real i = 0.4f * mass * radius * radius;
		inertia[0][0]	   = i;
		inertia[1][1]	   = i;
		inertia[2][2]	   = i;

		inverseInertiaTensor = Math::Inverse(inertia);
	}
	UpdateDerivedData();
}

void RigidBody::SetInertiaTensorForCapsuleCollider(const Math::real radius, const Math::real height) {
	if (inverseMass <= 0.0f) {
		inverseInertiaTensor = Math::RMat33(0.0f);
	} else {
		const Math::real mass = 1.0f / inverseMass;

		// 1. Calculate the volumes of the capsule components
		const Math::real volumeCylinder = Math::RPI * radius * radius * height;
		const Math::real volumeSphere	= (4.0f / 3.0f) * Math::RPI * radius * radius * radius;
		const Math::real totalVolume	= volumeCylinder + volumeSphere;

		// 2. Distribute the total mass among the shapes based on their volume ratio
		const Math::real massCylinder = mass * (volumeCylinder / totalVolume);
		const Math::real massSphere	  = mass * (volumeSphere / totalVolume);  // Total mass of the two hemispheres

		// 3. Inertia around the Y-Axis (Spine)
		// Cylinder Y-axis inertia + Sphere Y-axis inertia
		const Math::real inertiaY = (0.5f * massCylinder * radius * radius) + (0.4f * massSphere * radius * radius);

		// 4. Inertia around the X and Z Axes (using the Parallel Axis Theorem)
		const Math::real inertiaXCylinder = (1.0f / 12.0f) * massCylinder * (3.0f * radius * radius + height * height);

		// Inertia of the hemispheres displaced by 'height/2' from the center
		const Math::real inertiaXHemispheres =
			massSphere * (0.4f * radius * radius + 0.25f * height * height + 0.375f * height * radius);

		const Math::real inertiaX = inertiaXCylinder + inertiaXHemispheres;

		// 5. Build the inertia tensor matrix
		Math::RMat33 inertia(0.0f);
		inertia[0][0] = inertiaX;
		inertia[1][1] = inertiaY;  // The capsule extends along the Y-axis
		inertia[2][2] = inertiaX;  // Z-axis is symmetrical to X

		inverseInertiaTensor = Math::Inverse(inertia);
	}
	UpdateDerivedData();
}

void RigidBody::SetInertiaTensorForCylinderCollider(const Math::real radius, const Math::real height) {
	if (inverseMass <= 0.0f) {
		inverseInertiaTensor = Math::RMat33(0.0f);
	} else {
		const Math::real mass = 1.0f / inverseMass;

		// 1. Inertia around the Y-axis (central axis / spine of the cylinder)
		// Formula: I_y = 1/2 * m * r^2
		const Math::real inertiaY = 0.5f * mass * radius * radius;

		// 2. Inertia around the X and Z axes (transverse axes)
		// Formula: I_x = I_z = 1/12 * m * (3 * r^2 + h^2)
		const Math::real inertiaXZ = (1.0f / 12.0f) * mass * (3.0f * radius * radius + height * height);

		// 3. Build the inertia tensor matrix
		Math::RMat33 inertia(0.0f);
		inertia[0][0] = inertiaXZ;	// X-axis
		inertia[1][1] = inertiaY;	// Y-axis
		inertia[2][2] = inertiaXZ;	// Z-axis

		inverseInertiaTensor = Math::Inverse(inertia);
	}
	UpdateDerivedData();
}

void RigidBody::SetMassAndInertiaTensorForBoxCollider(const Math::real mass, const Math::RVec3 &extents) {
	if (mass <= 0.0f) {
		inverseMass			 = 0.0f;
		inverseInertiaTensor = Math::RMat33(0.0f);
	} else {
		inverseMass = 1.0f / mass;
		Math::RMat33 inertia(0.0f);

		inertia[0][0] = (1.0f / 3.0f) * mass * (extents.y * extents.y + extents.z * extents.z);
		inertia[1][1] = (1.0f / 3.0f) * mass * (extents.x * extents.x + extents.z * extents.z);
		inertia[2][2] = (1.0f / 3.0f) * mass * (extents.x * extents.x + extents.y * extents.y);

		inverseInertiaTensor = Math::Inverse(inertia);
	}
	UpdateDerivedData();
}

void RigidBody::SetMassAndInertiaTensorForSphereCollider(const Math::real mass, const Math::real radius) {
	if (mass <= 0.0f) {
		inverseMass			 = 0.0f;
		inverseInertiaTensor = Math::RMat33(0.0f);
	} else {
		inverseMass = 1.0f / mass;
		Math::RMat33 inertia(0.0f);

		// I = 2/5 * m * r^2
		const Math::real i = 0.4f * mass * radius * radius;
		inertia[0][0]	   = i;
		inertia[1][1]	   = i;
		inertia[2][2]	   = i;

		inverseInertiaTensor = Math::Inverse(inertia);
	}
	UpdateDerivedData();
}

void RigidBody::SetMassAndInertiaTensorForCapsuleCollider(const Math::real mass, const Math::real radius,
														  const Math::real height) {
	if (mass <= 0.0f) {
		inverseMass			 = 0.0f;
		inverseInertiaTensor = Math::RMat33(0.0f);
	} else {
		inverseMass = 1.0f / mass;

		const Math::real volumeCylinder = Math::PI * radius * radius * height;
		const Math::real volumeSphere	= (4.0f / 3.0f) * Math::PI * radius * radius * radius;
		const Math::real totalVolume	= volumeCylinder + volumeSphere;

		const Math::real massCylinder = mass * (volumeCylinder / totalVolume);
		const Math::real massSphere	  = mass * (volumeSphere / totalVolume);

		const Math::real inertiaY = (0.5f * massCylinder * radius * radius) + (0.4f * massSphere * radius * radius);

		const Math::real inertiaXCylinder = (1.0f / 12.0f) * massCylinder * (3.0f * radius * radius + height * height);
		const Math::real inertiaXHemispheres =
			massSphere * (0.4f * radius * radius + 0.25f * height * height + 0.375f * height * radius);

		const Math::real inertiaX = inertiaXCylinder + inertiaXHemispheres;

		Math::RMat33 inertia(0.0f);
		inertia[0][0] = inertiaX;
		inertia[1][1] = inertiaY;
		inertia[2][2] = inertiaX;

		inverseInertiaTensor = Math::Inverse(inertia);
	}
	UpdateDerivedData();
}

void RigidBody::SetMassAndInertiaTensorForCylinderCollider(const Math::real mass, const Math::real radius,
														   const Math::real height) {
	if (mass <= 0.0f) {
		inverseMass			 = 0.0f;
		inverseInertiaTensor = Math::RMat33(0.0f);
	} else {
		inverseMass = 1.0f / mass;

		// 1. Inertia around the Y-axis
		const Math::real inertiaY = 0.5f * mass * radius * radius;

		// 2. Inertia around the X and Z axes
		const Math::real inertiaXZ = (1.0f / 12.0f) * mass * (3.0f * radius * radius + height * height);

		// 3. Build the inertia tensor matrix
		Math::RMat33 inertia(0.0f);
		inertia[0][0] = inertiaXZ;
		inertia[1][1] = inertiaY;
		inertia[2][2] = inertiaXZ;

		inverseInertiaTensor = Math::Inverse(inertia);
	}
	UpdateDerivedData();
}
}  // namespace PE::Physics::Body::Components