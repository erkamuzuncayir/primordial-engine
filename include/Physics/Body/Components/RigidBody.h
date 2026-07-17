#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {
// TODO: Add this into physics config!
constexpr Math::real SLEEP_EPSILON = 0.3;

struct RigidBody {
	RigidBody() = default;
	explicit RigidBody(const Math::RVec3 position) : position(position) {}
	RigidBody(const Math::RVec3 position, const Math::RQuat orientation)
		: position(position), orientation(orientation) {}
	RigidBody(const Math::RVec3 position, const Math::RVec3 velocity) : position(position), velocity(velocity) {}
	RigidBody(const Math::RVec3 position, const Math::RVec3 velocity, const Math::RVec3 acceleration)
		: position(position), velocity(velocity), acceleration(acceleration) {}

	void UpdateDerivedData();
	void CalculateInverseInertiaTensorWorld();
	void ClearAccumulators();
	void Sleep();
	void WakeUp(Math::real newMotion);
	void SetMass(Math::real mass);
	void UpdateInertiaTensor(Math::real oldInverseMass, Math::real newInverseMass);
	void SetInertiaTensorForBoxCollider(const Math::RVec3 &extents);
	void SetInertiaTensorForSphereCollider(Math::real radius);
	void SetInertiaTensorForCapsuleCollider(Math::real radius, Math::real height);
	void SetInertiaTensorForCylinderCollider(Math::real radius, Math::real height);
	void SetMassAndInertiaTensorForBoxCollider(Math::real mass, const Math::RVec3 &extents);
	void SetMassAndInertiaTensorForSphereCollider(Math::real mass, Math::real radius);
	void SetMassAndInertiaTensorForCapsuleCollider(Math::real mass, Math::real radius, Math::real height);
	void SetMassAndInertiaTensorForCylinderCollider(Math::real mass, Math::real radius, Math::real height);

	Math::RVec3 position{0, 0, 0};
	Math::RVec3 velocity{0, 0, 0};
	Math::RVec3 acceleration{0, 0, 0};
	Math::RVec3 lastFrameAcceleration{0, 0, 0};
	Math::RVec3 forceAccum{0, 0, 0};
	Math::real	linearDamping{0.99f};
	Math::real	inverseMass{1};
	Math::real	gravityScale{1.0};

	Math::RQuat	 orientation{0, {0, 0, 0}};
	Math::RVec3	 angularVelocity{0, 0, 0};
	Math::RMat33 inverseInertiaTensor{0};
	Math::RMat33 inverseInertiaTensorWorld{0};
	Math::RVec3	 torqueAccum{0, 0, 0};
	Math::real	 angularDamping{0.99f};
	Math::real	 motion{2.0 * SLEEP_EPSILON};

	bool isAwake  = true;
	bool canSleep = true;
	bool isDirty  = true;
};
}  // namespace PE::Physics::Body::Components