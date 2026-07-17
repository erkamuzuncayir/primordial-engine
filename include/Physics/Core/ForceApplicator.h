#pragma once
#include "Math/Math.h"
#include "Physics/Body/Components/RigidBody.h"
#include "Physics/Particle/Components/PointMass.h"

namespace PE::Physics::Core {

class ForceApplicator {
public:
	static void ApplyForce(Body::Components::RigidBody &rb, const Math::RVec3 &force) {
		rb.forceAccum += force;
		rb.isAwake = true;
	}

	static void ApplyForce(Particle::Components::PointMass &pm, const Math::RVec3 &force) { pm.forceAccum += force; }

	static void ApplyForceAtPoint(Body::Components::RigidBody &rb, const Math::RVec3 &force,
								  const Math::RVec3 &worldPoint) {
		const Math::RVec3 pt = worldPoint - rb.position;

		rb.forceAccum += force;
		rb.torqueAccum += Math::Cross(pt, force);
		rb.isAwake = true;
	}

	static void ApplyForceAtLocalPoint(Body::Components::RigidBody &rb, const Math::RVec3 &force,
									   const Math::RVec3 &localPoint) {
		const Math::RVec3 worldPoint = (rb.orientation * localPoint) + rb.position;
		ApplyForceAtPoint(rb, force, worldPoint);
	}

	static void ApplyLocalForceAtLocalPoint(Body::Components::RigidBody &rb, const Math::RVec3 &localForce,
											const Math::RVec3 &localPoint) {
		const Math::RVec3 worldPoint = (rb.orientation * localPoint) + rb.position;
		const Math::RVec3 worldForce = (rb.orientation * localForce);
		ApplyForceAtPoint(rb, worldForce, worldPoint);
	}
};
}  // namespace PE::Physics::Core