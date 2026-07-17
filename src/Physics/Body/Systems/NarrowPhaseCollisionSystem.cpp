#include "Physics/Body/Systems/NarrowPhaseCollisionSystem.h"

#include <format>

#include "Core/EngineConfig.h"
#include "Physics/Body/Components/CapsuleCollider.h"
#include "Physics/Body/Components/CylinderCollider.h"
#include "Physics/Body/Components/SphereCollider.h"
#include "Physics/Body/Systems/BroadPhaseCollisionSystem.h"
#include "Physics/Body/Utilities.h"
#include "Scene/Components/Transform.h"

namespace PE::Physics::Body::Components {
struct BoxCollider;
}

namespace PE::Physics::Body::Systems {
void NarrowPhaseCollisionSystem::Initialize(ECS::ECSManager *ecsManager, const Core::EngineConfig &config,
											PhysicsMaterialSystem *physicsMaterialSystem) {
	ref_eM					  = ecsManager;
	ref_physicsMaterialSystem = physicsMaterialSystem;
	m_staticContacts.reserve(config.maxEntityCount);
	m_kinematicContacts.reserve(config.maxEntityCount);
	m_rigidBodyContacts.reserve(config.maxEntityCount);
}

void NarrowPhaseCollisionSystem::Shutdown() {}

void NarrowPhaseCollisionSystem::OnUpdate(const CollisionPairs &collisionPairs, const Math::real dt) {
	m_staticContacts.clear();
	m_kinematicContacts.clear();
	m_rigidBodyContacts.clear();
	DetectCollisions(collisionPairs);
	ContactResolver::ResolveContacts(ref_eM, m_staticContacts, m_kinematicContacts, m_rigidBodyContacts, dt);
}

void NarrowPhaseCollisionSystem::DetectCollisions(const CollisionPairs &collisionPairs) {
	// Lambda for Phase Iteration
	auto processThreePhases = [&](const Components::ColliderShape typeA, const Components::ColliderShape typeB,
								  auto func, bool flip) {
		ProcessCollisions(collisionPairs.dynamicVsStatic[static_cast<uint32_t>(typeA)][static_cast<uint32_t>(typeB)],
						  m_staticContacts, func, flip);
		ProcessCollisions(collisionPairs.dynamicVsKinematic[static_cast<uint32_t>(typeA)][static_cast<uint32_t>(typeB)],
						  m_kinematicContacts, func, flip);
		ProcessCollisions<RigidBodyContact>(
			collisionPairs.dynamicVsDynamic[static_cast<uint32_t>(typeA)][static_cast<uint32_t>(typeB)],
			m_rigidBodyContacts, func, flip);
	};

	using namespace Components;

	// --- Box (0) ---
	processThreePhases(
		ColliderShape::Box, ColliderShape::Box,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return BoxAndBox(a, b, n, pt, p); }, false);
	processThreePhases(
		ColliderShape::Box, ColliderShape::Sphere,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return BoxAndSphere(a, b, n, pt, p); }, false);
	processThreePhases(
		ColliderShape::Box, ColliderShape::Capsule,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return CapsuleAndBox(b, a, n, pt, p); }, true);
	processThreePhases(
		ColliderShape::Box, ColliderShape::Cylinder,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return CylinderAndBox(b, a, n, pt, p); }, true);

	// --- Sphere (1) ---
	processThreePhases(
		ColliderShape::Sphere, ColliderShape::Box,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return BoxAndSphere(b, a, n, pt, p); }, true);
	processThreePhases(
		ColliderShape::Sphere, ColliderShape::Sphere,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return SphereAndSphere(a, b, n, pt, p); }, false);
	processThreePhases(
		ColliderShape::Sphere, ColliderShape::Capsule,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return SphereAndCapsule(a, b, n, pt, p); }, false);
	processThreePhases(
		ColliderShape::Sphere, ColliderShape::Cylinder,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return SphereAndCylinder(a, b, n, pt, p); }, false);

	// --- Capsule (2) ---
	processThreePhases(
		ColliderShape::Capsule, ColliderShape::Box,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return CapsuleAndBox(a, b, n, pt, p); }, false);
	processThreePhases(
		ColliderShape::Capsule, ColliderShape::Sphere,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return SphereAndCapsule(b, a, n, pt, p); }, true);
	processThreePhases(
		ColliderShape::Capsule, ColliderShape::Capsule,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return CapsuleAndCapsule(a, b, n, pt, p); }, false);
	processThreePhases(
		ColliderShape::Capsule, ColliderShape::Cylinder,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return CapsuleAndCylinder(a, b, n, pt, p); }, false);

	// --- Cylinder (3) ---
	processThreePhases(
		ColliderShape::Cylinder, ColliderShape::Box,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return CylinderAndBox(a, b, n, pt, p); }, false);
	processThreePhases(
		ColliderShape::Cylinder, ColliderShape::Sphere,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return SphereAndCylinder(b, a, n, pt, p); }, true);
	processThreePhases(
		ColliderShape::Cylinder, ColliderShape::Capsule,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return CapsuleAndCylinder(b, a, n, pt, p); }, true);
	processThreePhases(
		ColliderShape::Cylinder, ColliderShape::Cylinder,
		[this](auto a, auto b, auto &n, auto &pt, auto &p) { return CylinderAndCylinder(a, b, n, pt, p); }, false);

	// --- Containers ---
	auto processAllContainerPhases = [&]<typename TSolid, typename TContainer>(TSolid solidDummy, TContainer contDummy,
																			   const ColliderShape solidIdx,
																			   const ColliderShape contIdx) {
		auto funcDynSolid = [this](const ECS::EntityID idOne, const ECS::EntityID idTwo, Math::RVec3 &n,
								   Math::RVec3 &pt, Math::real &p) {
			return ResolveContainer<TSolid, TContainer>(idOne, idTwo, n, pt, p);
		};

		auto funcDynCont = [this](const ECS::EntityID idOne, const ECS::EntityID idTwo, Math::RVec3 &n, Math::RVec3 &pt,
								  Math::real &p) {
			return ResolveContainer<TSolid, TContainer>(idTwo, idOne, n, pt, p);
		};

		ProcessCollisions(
			collisionPairs.dynSolidVsStatCont[static_cast<uint32_t>(solidIdx)][static_cast<uint32_t>(contIdx)],
			m_staticContacts, funcDynSolid, false);
		ProcessCollisions(
			collisionPairs.dynContVsStatSolid[static_cast<uint32_t>(solidIdx)][static_cast<uint32_t>(contIdx)],
			m_staticContacts, funcDynCont, true);
		ProcessCollisions(
			collisionPairs.dynSolidVsKinCont[static_cast<uint32_t>(solidIdx)][static_cast<uint32_t>(contIdx)],
			m_kinematicContacts, funcDynSolid, false);
		ProcessCollisions(
			collisionPairs.dynContVsKinSolid[static_cast<uint32_t>(solidIdx)][static_cast<uint32_t>(contIdx)],
			m_kinematicContacts, funcDynCont, true);
		ProcessCollisions<RigidBodyContact>(
			collisionPairs.dynSolidVsDynCont[static_cast<uint32_t>(solidIdx)][static_cast<uint32_t>(contIdx)],
			m_rigidBodyContacts, funcDynSolid, false);
	};

	// Solid: BOX (0)
	processAllContainerPhases(BoxCollider{}, BoxCollider{}, ColliderShape::Box, ColliderShape::Box);
	processAllContainerPhases(BoxCollider{}, SphereCollider{}, ColliderShape::Box, ColliderShape::Sphere);
	processAllContainerPhases(BoxCollider{}, CapsuleCollider{}, ColliderShape::Box, ColliderShape::Capsule);
	processAllContainerPhases(BoxCollider{}, CylinderCollider{}, ColliderShape::Box, ColliderShape::Cylinder);

	// Solid: SPHERE (1)
	processAllContainerPhases(SphereCollider{}, BoxCollider{}, ColliderShape::Sphere, ColliderShape::Box);
	processAllContainerPhases(SphereCollider{}, SphereCollider{}, ColliderShape::Sphere, ColliderShape::Sphere);
	processAllContainerPhases(SphereCollider{}, CapsuleCollider{}, ColliderShape::Sphere, ColliderShape::Capsule);
	processAllContainerPhases(SphereCollider{}, CylinderCollider{}, ColliderShape::Sphere, ColliderShape::Cylinder);

	// Solid: CAPSULE (2)
	processAllContainerPhases(CapsuleCollider{}, BoxCollider{}, ColliderShape::Capsule, ColliderShape::Box);
	processAllContainerPhases(CapsuleCollider{}, SphereCollider{}, ColliderShape::Capsule, ColliderShape::Sphere);
	processAllContainerPhases(CapsuleCollider{}, CapsuleCollider{}, ColliderShape::Capsule, ColliderShape::Capsule);
	processAllContainerPhases(CapsuleCollider{}, CylinderCollider{}, ColliderShape::Capsule, ColliderShape::Cylinder);

	// Solid: CYLINDER (3)
	processAllContainerPhases(CylinderCollider{}, BoxCollider{}, ColliderShape::Cylinder, ColliderShape::Box);
	processAllContainerPhases(CylinderCollider{}, SphereCollider{}, ColliderShape::Cylinder, ColliderShape::Sphere);
	processAllContainerPhases(CylinderCollider{}, CapsuleCollider{}, ColliderShape::Cylinder, ColliderShape::Capsule);
	processAllContainerPhases(CylinderCollider{}, CylinderCollider{}, ColliderShape::Cylinder, ColliderShape::Cylinder);
}

bool NarrowPhaseCollisionSystem::SphereAndSphere(const ECS::EntityID idOne, const ECS::EntityID idTwo,
												 Math::RVec3 &outNormal, Math::RVec3 &outPoint,
												 Math::real &outPenetration) const {
	const Components::SphereCollider   &colOne = *ref_eM->GetTComponent<Components::SphereCollider>(idOne);
	const Components::SphereCollider   &colTwo = *ref_eM->GetTComponent<Components::SphereCollider>(idTwo);
	const Scene::Components::Transform &tfOne  = *ref_eM->GetTComponent<Scene::Components::Transform>(idOne);
	const Scene::Components::Transform &tfTwo  = *ref_eM->GetTComponent<Scene::Components::Transform>(idTwo);

	const Math::RVec3 centerOne = tfOne.position + (tfOne.orientation * (colOne.localOffset * tfOne.scale));
	const Math::RVec3 centerTwo = tfTwo.position + (tfTwo.orientation * (colTwo.localOffset * tfTwo.scale));

	const Math::RVec3 midline = centerOne - centerTwo;
	const Math::real  size	  = Math::Length(midline);

	if (size <= 0.0f || size >= colOne.worldRadius + colTwo.worldRadius) return false;

	const Math::RVec3 normal = midline * (static_cast<Math::real>(1.0) / size);

	outNormal	   = normal;
	outPoint	   = centerOne - (outNormal * colOne.worldRadius);
	outPenetration = (colOne.worldRadius + colTwo.worldRadius - size);
	return true;
}

bool NarrowPhaseCollisionSystem::SphereAndCapsule(const ECS::EntityID sphereID, const ECS::EntityID capID,
												  Math::RVec3 &outNormal, Math::RVec3 &outPoint,
												  Math::real &outPenetration) const {
	const Scene::Components::Transform &sphereTf = *ref_eM->GetTComponent<Scene::Components::Transform>(sphereID);
	const Scene::Components::Transform &capTf	 = *ref_eM->GetTComponent<Scene::Components::Transform>(capID);

	const Components::SphereCollider  &sphereCol = *ref_eM->GetTComponent<Components::SphereCollider>(sphereID);
	const Components::CapsuleCollider &capCol	 = *ref_eM->GetTComponent<Components::CapsuleCollider>(capID);

	// 1. Calculate World Centers
	Math::RVec3 sphereCenter = sphereTf.position + (sphereTf.orientation * (sphereCol.localOffset * sphereTf.scale));
	Math::RVec3 capCenter	 = capTf.position + (capTf.orientation * (capCol.localOffset * capTf.scale));

	// 2. Find the Capsule's internal line segment (A to B) in world space
	Math::RVec3 upAxis		= capTf.orientation * Math::RVec3(0, 1, 0);
	Math::RVec3 capBottomPt = capCenter - upAxis * capCol.worldHalfHeight;
	Math::RVec3 capTopPt	= capCenter + upAxis * capCol.worldHalfHeight;

	// 3. Find the closest point on the line segment A-B to the sphere center
	Math::RVec3 capUpDir	  = capTopPt - capBottomPt;
	Math::real	t			  = Math::Dot(sphereCenter - capBottomPt, capUpDir) / Math::Dot(capUpDir, capUpDir);
	t						  = Math::Clamp(t, static_cast<Math::real>(0.0), static_cast<Math::real>(1.0));
	Math::RVec3 closestLinePt = capBottomPt + capUpDir * t;

	// 4. Check distance from the closest point to the sphere center
	Math::RVec3 dir		  = sphereCenter - closestLinePt;
	Math::real	distSq	  = Math::Dot(dir, dir);
	Math::real	radiusSum = sphereCol.worldRadius + capCol.worldRadius;

	// Early out if they are too far apart
	if (distSq > radiusSum * radiusSum) return false;

	// 5. Calculate Contact Data
	if (Math::real dist = Math::Sqrt(distSq); dist <= Math::REpsilon) {
		// Extreme edge case: Centers perfectly overlap
		outNormal	   = upAxis;
		outPenetration = radiusSum;
	} else {
		outNormal	   = dir * (static_cast<Math::real>(1.0) / dist);  // Points from Capsule to Sphere
		outPenetration = radiusSum - dist;
	}

	// The contact point is on the surface of the capsule
	outPoint = closestLinePt + outNormal * capCol.worldRadius;

	return true;
}

bool NarrowPhaseCollisionSystem::SphereAndCylinder(const ECS::EntityID sphereID, const ECS::EntityID cylID,
												   Math::RVec3 &outNormal, Math::RVec3 &outPoint,
												   Math::real &outPenetration) const {
	const Scene::Components::Transform &sphereTf = *ref_eM->GetTComponent<Scene::Components::Transform>(sphereID);
	const Scene::Components::Transform &cylTf	 = *ref_eM->GetTComponent<Scene::Components::Transform>(cylID);

	const Components::SphereCollider   &sphereCol = *ref_eM->GetTComponent<Components::SphereCollider>(sphereID);
	const Components::CylinderCollider &cylCol	  = *ref_eM->GetTComponent<Components::CylinderCollider>(cylID);

	// 1. Calculate World Centers
	Math::RVec3 sphereCenter = sphereTf.position + (sphereTf.orientation * (sphereCol.localOffset * sphereTf.scale));
	Math::RVec3 cylCenter	 = cylTf.position + (cylTf.orientation * (cylCol.localOffset * cylTf.scale));

	// 2. Project Sphere Center into the Cylinder's local frame
	Math::RVec3 cylUpAxis	   = cylTf.orientation * Math::RVec3(0, 1, 0);
	Math::RVec3 dirCylToSphere = sphereCenter - cylCenter;

	// Axial distance (Y-axis)
	Math::real y = Math::Dot(dirCylToSphere, cylUpAxis);

	// Radial vector (X/Z plane)
	Math::RVec3 radialVec = dirCylToSphere - cylUpAxis * y;
	Math::real	x		  = Math::Length(radialVec);

	// 3. Find the closest point on the cylinder's 2D cross-section (a rectangle)
	Math::real px = Math::Clamp(x, static_cast<Math::real>(0.0), cylCol.worldRadius);
	Math::real py = Math::Clamp(y, -cylCol.worldHalfHeight, cylCol.worldHalfHeight);

	// 4. Calculate the distance from the sphere to the closest point
	Math::real dx	  = x - px;
	Math::real dy	  = y - py;
	Math::real distSq = dx * dx + dy * dy;

	if (distSq > sphereCol.worldRadius * sphereCol.worldRadius) return false;

	Math::real	dist = Math::Sqrt(distSq);
	Math::RVec3 normal;

	// 5. Calculate Contact Data
	if (dist > Math::REpsilon) {
		// Normal collision (shallow penetration)
		Math::RVec3 radialDir =
			(x > Math::REpsilon) ? (radialVec * (static_cast<Math::real>(1.0) / x)) : Math::RVec3(1, 0, 0);
		normal		   = (radialDir * dx + cylUpAxis * dy) * (static_cast<Math::real>(1.0) / dist);
		outPenetration = sphereCol.worldRadius - dist;
	} else {
		// Deep Penetration (Sphere center is inside the cylinder)
		// Find the shortest path to push the sphere out
		Math::real penX = cylCol.worldRadius - x;
		Math::real penY = cylCol.worldHalfHeight - Math::Abs(y);

		if (penX < penY) {
			// Push out radially (side of the cylinder)
			normal = (x > Math::REpsilon) ? (radialVec * (static_cast<Math::real>(1.0) / x)) : Math::RVec3(1, 0, 0);
			outPenetration = penX + sphereCol.worldRadius;
		} else {
			// Push out axially (top or bottom caps)
			normal		   = (y > 0.0f) ? cylUpAxis : (cylUpAxis * static_cast<Math::real>(-1.0));
			outPenetration = penY + sphereCol.worldRadius;
		}
	}

	outNormal = normal;	 // Points from Cylinder to Sphere

	// Deepest point of the sphere touching the cylinder
	outPoint = sphereCenter - normal * sphereCol.worldRadius;

	return true;
}

bool NarrowPhaseCollisionSystem::CylinderAndCylinder(const ECS::EntityID cylOneID, const ECS::EntityID cylTwoID,
													 Math::RVec3 &outNormal, Math::RVec3 &outPoint,
													 Math::real &outPenetration) const {
	const Scene::Components::Transform &tfOne = *ref_eM->GetTComponent<Scene::Components::Transform>(cylOneID);
	const Scene::Components::Transform &tfTwo = *ref_eM->GetTComponent<Scene::Components::Transform>(cylTwoID);

	const Components::CylinderCollider &colOne = *ref_eM->GetTComponent<Components::CylinderCollider>(cylOneID);
	const Components::CylinderCollider &colTwo = *ref_eM->GetTComponent<Components::CylinderCollider>(cylTwoID);

	// 1. Calculate World Centers and Up Vectors
	Math::RVec3 centerOne = tfOne.position + (tfOne.orientation * (colOne.localOffset * tfOne.scale));
	Math::RVec3 centerTwo = tfTwo.position + (tfTwo.orientation * (colTwo.localOffset * tfTwo.scale));

	Math::RVec3 upOne = tfOne.orientation * Math::RVec3(0, 1, 0);
	Math::RVec3 upTwo = tfTwo.orientation * Math::RVec3(0, 1, 0);

	// 2. Find Closest Points between their central line segments
	Math::RVec3 bottomPtOne = centerOne - upOne * colOne.worldHalfHeight;
	Math::RVec3 topPtOne	= centerOne + upOne * colOne.worldHalfHeight;
	Math::RVec3 bottomPtTwo = centerTwo - upTwo * colTwo.worldHalfHeight;
	Math::RVec3 topPtTwo	= centerTwo + upTwo * colTwo.worldHalfHeight;

	Math::RVec3 ptOne{};
	Math::RVec3 ptTwo{};
	ClosestPointsBetweenSegments(bottomPtOne, topPtOne, bottomPtTwo, topPtTwo, ptOne, ptTwo);

	// 3. Define the 3 SAT Test Axes
	Math::RVec3 axis3 = ptOne - ptTwo;

	if (Math::real distSq = Math::Dot(axis3, axis3); distSq > Math::REpsilon) {
		axis3 = axis3 * (static_cast<Math::real>(1.0) / Math::Sqrt(distSq));
	} else {
		axis3 = Math::RVec3(0, 1, 0);  // They are perfectly merged/parallel
	}

	Math::RVec3 axes[3] = {upOne, upTwo, axis3};

	Math::real	bestPenetration = std::numeric_limits<Math::real>::max();
	Math::RVec3 bestAxis		= Math::RVec3(0, 1, 0);

	// 4. Test the Axes
	for (int i = 0; i < 3; ++i) {
		Math::RVec3 axis = axes[i];
		if (Math::Dot(axis, axis) < Math::REpsilon) continue;

		axis = Math::Normalize(axis);

		// Project Cylinder 1 onto the axis
		Math::real dot1	  = Math::Dot(axis, upOne);
		Math::real sinSq1 = static_cast<Math::real>(1.0) - dot1 * dot1;
		Math::real sin1	  = (sinSq1 > 0.0f) ? Math::Sqrt(sinSq1) : 0.0f;
		Math::real proj1  = colOne.worldHalfHeight * Math::Abs(dot1) + colOne.worldRadius * sin1;

		// Project Cylinder 2 onto the axis
		Math::real dot2	  = Math::Dot(axis, upTwo);
		Math::real sinSq2 = static_cast<Math::real>(1.0) - dot2 * dot2;
		Math::real sin2	  = (sinSq2 > 0.0f) ? Math::Sqrt(sinSq2) : 0.0f;
		Math::real proj2  = colTwo.worldHalfHeight * Math::Abs(dot2) + colTwo.worldRadius * sin2;

		// Calculate distance between centers along this axis
		Math::real dist = Math::Abs(Math::Dot(centerOne - centerTwo, axis));

		// Penetration formula
		Math::real penetration = (proj1 + proj2) - dist;

		// If penetration is negative on ANY axis, a gap exists. They are NOT colliding.
		if (penetration < 0.0f) {
			return false;
		}

		// Keep the axis with the smallest penetration (The Axis of Least Resistance)
		if (penetration < bestPenetration) {
			bestPenetration = penetration;
			bestAxis		= axis;
		}
	}

	// 5. Build Final Contact Data
	// Ensure the normal points from Object Two to Object One
	if (Math::Dot(bestAxis, centerOne - centerTwo) < 0.0f) {
		bestAxis = bestAxis * static_cast<Math::real>(-1.0);
	}

	outNormal	   = bestAxis;
	outPenetration = bestPenetration;

	// Contact point is the support point on Cylinder Two pushed into Cylinder One
	outPoint = GetCylinderSupportPoint(centerTwo, upTwo, colTwo.worldHalfHeight, colTwo.worldRadius, bestAxis);

	return true;
}

Math::RVec3 NarrowPhaseCollisionSystem::GetCylinderSupportPoint(const Math::RVec3 &center, const Math::RVec3 &upDir,
																const Math::real halfHeight, const Math::real radius,
																const Math::RVec3 &dir) const {
	const Math::real dotY  = Math::Dot(dir, upDir);
	const Math::real signY = (dotY > 0.0f) ? 1.0f : -1.0f;

	// Project the direction onto the cylinder's flat XZ plane
	Math::RVec3 radialDir = dir - upDir * dotY;

	if (const Math::real radialLenSq = Math::Dot(radialDir, radialDir); radialLenSq > Math::REpsilon) {
		radialDir = radialDir * (static_cast<Math::real>(1.0) / Math::Sqrt(radialLenSq));
	} else {
		// Fallback: If pushing perfectly straight down onto the flat cap, pick an arbitrary edge
		Math::RVec3 fallback = Math::Cross(upDir, Math::RVec3(1, 0, 0));
		if (Math::Dot(fallback, fallback) < Math::REpsilon) {
			fallback = Math::Cross(upDir, Math::RVec3(0, 1, 0));
		}
		radialDir = Math::Normalize(fallback);
	}

	// Support point = Center + Height Offset + Radius Offset
	return center + (upDir * (halfHeight * signY)) + (radialDir * radius);
}

bool NarrowPhaseCollisionSystem::CylinderAndBox(const ECS::EntityID cylID, const ECS::EntityID boxID,
												Math::RVec3 &outNormal, Math::RVec3 &outPoint,
												Math::real &outPenetration) const {
	const Scene::Components::Transform &cylinderTransform =
		*ref_eM->GetTComponent<Scene::Components::Transform>(cylID);
	const Scene::Components::Transform &boxTransform	 = *ref_eM->GetTComponent<Scene::Components::Transform>(boxID);
	const Components::CylinderCollider &cylinderCollider = *ref_eM->GetTComponent<Components::CylinderCollider>(cylID);
	const Components::BoxCollider	   &boxCollider		 = *ref_eM->GetTComponent<Components::BoxCollider>(boxID);

	// 1. Calculate absolute world centers
	Math::RVec3 cylinderCenter =
		cylinderTransform.position +
		(cylinderTransform.orientation * (cylinderCollider.localOffset * cylinderTransform.scale));
	Math::RVec3 boxCenter =
		boxTransform.position + (boxTransform.orientation * (boxCollider.localOffset * boxTransform.scale));

	// 2. Define the Cylinder's central line segment (spine) in world space
	Math::RVec3 cylinderUpAxis		= cylinderTransform.orientation * Math::RVec3(0, 1, 0);
	Math::RVec3 cylinderBottomPoint = cylinderCenter - cylinderUpAxis * cylinderCollider.worldHalfHeight;
	Math::RVec3 cylinderTopPoint	= cylinderCenter + cylinderUpAxis * cylinderCollider.worldHalfHeight;

	// 3. Transform the Cylinder's line segment into the Box's Local Space
	Math::RQuat boxInverseRotation	= Math::Conjugate(boxTransform.orientation);
	Math::RVec3 localCylinderBottom = boxInverseRotation * (cylinderBottomPoint - boxCenter);
	Math::RVec3 localCylinderTop	= boxInverseRotation * (cylinderTopPoint - boxCenter);

	// 4. Alternating Projections: Find the closest points between the line segment and the AABB (Box)
	Math::RVec3 closestLocalCylinderPoint = (localCylinderBottom + localCylinderTop) * 0.5f;  // Start at the middle
	Math::RVec3 closestLocalBoxPoint;

	// Bounce back and forth 3 times to converge on the absolute closest points
	for (int i = 0; i < 3; ++i) {
		// Project cylinder point onto the Box (Clamp to AABB)
		closestLocalBoxPoint =
			Math::Clamp(closestLocalCylinderPoint, -boxCollider.worldHalfExtents, boxCollider.worldHalfExtents);
		// Project box point back onto the Cylinder's line segment
		closestLocalCylinderPoint = ClosestPointOnSegment(localCylinderBottom, localCylinderTop, closestLocalBoxPoint);
	}

	// Convert the found Box point back to World Space
	Math::RVec3 closestWorldBoxPoint = boxCenter + (boxTransform.orientation * closestLocalBoxPoint);

	// 5. Treat the closest Box point as a "Sphere with Radius 0" and test against the Cylinder
	Math::RVec3 boxPointToCylinderCenter = closestWorldBoxPoint - cylinderCenter;

	// Axial distance (height)
	Math::real verticalDist = Math::Dot(boxPointToCylinderCenter, cylinderUpAxis);

	// Radial vector and distance (width)
	Math::RVec3 radialVector = boxPointToCylinderCenter - cylinderUpAxis * verticalDist;
	Math::real	radialDist	 = Math::Length(radialVector);

	// Find the closest point on the cylinder's surface mathematically
	Math::real closestCylRadialDist =
		Math::Clamp(radialDist, static_cast<Math::real>(0.0), cylinderCollider.worldRadius);
	Math::real closestCylVerticalDist =
		Math::Clamp(verticalDist, -cylinderCollider.worldHalfHeight, cylinderCollider.worldHalfHeight);

	// Calculate the distance between the Box point and the Cylinder surface
	Math::real diffX = radialDist - closestCylRadialDist;
	Math::real diffY = verticalDist - closestCylVerticalDist;

	// If the distance is greater than 0, they are not touching!
	if (Math::real distanceSquared = diffX * diffX + diffY * diffY; distanceSquared > Math::REpsilon) return false;

	// 6. Deep Penetration Resolution (They are colliding)
	Math::real penetrationSide = cylinderCollider.worldRadius - radialDist;
	Math::real penetrationCap  = cylinderCollider.worldHalfHeight - Math::Abs(verticalDist);

	if (penetrationSide < penetrationCap) {
		// SIDE COLLISION: Push out radially
		Math::RVec3 radialDir = (radialDist > Math::REpsilon)
									? (radialVector * (static_cast<Math::real>(-1.0) / radialDist))
									: Math::RVec3(-1, 0, 0);
		outNormal			  = radialDir;
		outPenetration		  = penetrationSide;
	} else {
		// CAP COLLISION: Push out using the BOX'S face normal, NOT the cylinder's spine!
		// We find which face of the box the cylinder hit by checking the clamped local point
		Math::RVec3 localBoxNormal(0, 0, 0);
		Math::real	dx = boxCollider.worldHalfExtents.x - Math::Abs(closestLocalBoxPoint.x);
		Math::real	dy = boxCollider.worldHalfExtents.y - Math::Abs(closestLocalBoxPoint.y);
		Math::real	dz = boxCollider.worldHalfExtents.z - Math::Abs(closestLocalBoxPoint.z);

		if (dx <= dy && dx <= dz) {
			localBoxNormal.x = (closestLocalBoxPoint.x > 0.0f) ? 1.0f : -1.0f;
		} else if (dy <= dx && dy <= dz) {
			localBoxNormal.y = (closestLocalBoxPoint.y > 0.0f) ? 1.0f : -1.0f;
		} else {
			localBoxNormal.z = (closestLocalBoxPoint.z > 0.0f) ? 1.0f : -1.0f;
		}

		// The normal points directly away from the Box face (e.g., straight UP for a floor)
		outNormal	   = boxTransform.orientation * localBoxNormal;
		outPenetration = penetrationCap;
	}

	outPoint = closestWorldBoxPoint;
	return true;
}

bool NarrowPhaseCollisionSystem::CapsuleAndCylinder(const ECS::EntityID capID, const ECS::EntityID cylID,
													Math::RVec3 &outNormal, Math::RVec3 &outPoint,
													Math::real &outPenetration) const {
	const Scene::Components::Transform &capsuleTransform = *ref_eM->GetTComponent<Scene::Components::Transform>(capID);
	const Scene::Components::Transform &cylinderTransform =
		*ref_eM->GetTComponent<Scene::Components::Transform>(cylID);

	const Components::CapsuleCollider  &capsuleCollider	 = *ref_eM->GetTComponent<Components::CapsuleCollider>(capID);
	const Components::CylinderCollider &cylinderCollider = *ref_eM->GetTComponent<Components::CylinderCollider>(cylID);

	// 1. Calculate World Centers and Local Up-Axes
	Math::RVec3 capsuleCenter = capsuleTransform.position +
								(capsuleTransform.orientation * (capsuleCollider.localOffset * capsuleTransform.scale));
	Math::RVec3 cylinderCenter =
		cylinderTransform.position +
		(cylinderTransform.orientation * (cylinderCollider.localOffset * cylinderTransform.scale));

	Math::RVec3 capsuleUpAxis  = capsuleTransform.orientation * Math::RVec3(0, 1, 0);
	Math::RVec3 cylinderUpAxis = cylinderTransform.orientation * Math::RVec3(0, 1, 0);

	// 2. Extract the internal line segments (spines) of both shapes
	Math::RVec3 capsuleSegmentBottom  = capsuleCenter - capsuleUpAxis * capsuleCollider.worldHalfHeight;
	Math::RVec3 capsuleSegmentTop	  = capsuleCenter + capsuleUpAxis * capsuleCollider.worldHalfHeight;
	Math::RVec3 cylinderSegmentBottom = cylinderCenter - cylinderUpAxis * cylinderCollider.worldHalfHeight;
	Math::RVec3 cylinderSegmentTop	  = cylinderCenter + cylinderUpAxis * cylinderCollider.worldHalfHeight;

	// Find the closest points between these two floating spines
	Math::RVec3 closestCapsulePoint;
	Math::RVec3 closestCylinderPoint;
	ClosestPointsBetweenSegments(capsuleSegmentBottom, capsuleSegmentTop, cylinderSegmentBottom, cylinderSegmentTop,
								 closestCapsulePoint, closestCylinderPoint);

	// 3. Define the 3 Critical Axes for the Separating Axis Theorem (SAT)
	Math::RVec3 bridgeAxis = closestCapsulePoint - closestCylinderPoint;  // The shortest path between spines

	if (Math::real bridgeLengthSquared = Math::Dot(bridgeAxis, bridgeAxis); bridgeLengthSquared > Math::REpsilon) {
		bridgeAxis = bridgeAxis * (static_cast<Math::real>(1.0) / Math::Sqrt(bridgeLengthSquared));	 // Normalize
	} else {
		bridgeAxis = Math::RVec3(0, 1, 0);	// Fallback if spines are perfectly intersecting
	}

	// The only 3 directions we ever need to test
	Math::RVec3 testAxes[3] = {capsuleUpAxis, cylinderUpAxis, bridgeAxis};

	Math::real	bestPenetration = std::numeric_limits<Math::real>::max();
	Math::RVec3 bestAxis		= Math::RVec3(0, 1, 0);

	// 4. SAT Projections
	for (int i = 0; i < 3; ++i) {
		Math::RVec3 axis = testAxes[i];
		if (Math::Dot(axis, axis) < Math::REpsilon) continue;  // Skip invalid axes

		axis = Math::Normalize(axis);

		// Project Capsule onto the axis
		Math::real capsuleCosAngle = Math::Dot(axis, capsuleUpAxis);
		Math::real capsuleProjection =
			capsuleCollider.worldHalfHeight * Math::Abs(capsuleCosAngle) + capsuleCollider.worldRadius;

		// Project Cylinder onto the axis
		Math::real cylinderCosAngle	  = Math::Dot(axis, cylinderUpAxis);
		Math::real cylinderSinSquared = static_cast<Math::real>(1.0) - cylinderCosAngle * cylinderCosAngle;
		Math::real cylinderSin		  = (cylinderSinSquared > 0.0f) ? Math::Sqrt(cylinderSinSquared) : 0.0f;
		Math::real cylinderProjection =
			cylinderCollider.worldHalfHeight * Math::Abs(cylinderCosAngle) + cylinderCollider.worldRadius * cylinderSin;

		// Calculate how far apart the two centers are along this specific axis
		Math::real centerDistanceOnAxis = Math::Abs(Math::Dot(capsuleCenter - cylinderCenter, axis));

		// Penetration = Sum of Projections - Distance Between Centers
		Math::real penetration = (capsuleProjection + cylinderProjection) - centerDistanceOnAxis;

		// If penetration is negative, there is a gap. No collision.
		if (penetration < 0.0f) return false;

		// Keep the axis that pushes the objects out the least
		if (penetration < bestPenetration) {
			bestPenetration = penetration;
			bestAxis		= axis;
		}
	}

	// 5. Build Final Contact Data for the Physics Solver
	// Ensure the push normal always points from Cylinder to Capsule
	if (Math::Dot(bestAxis, capsuleCenter - cylinderCenter) < 0.0f) {
		bestAxis = bestAxis * static_cast<Math::real>(-1.0);
	}

	outNormal	   = bestAxis;
	outPenetration = bestPenetration;

	// The contact point is the support point on the Cylinder, pushed into the Capsule
	outPoint = GetCylinderSupportPoint(cylinderCenter, cylinderUpAxis, cylinderCollider.worldHalfHeight,
									   cylinderCollider.worldRadius, bestAxis);

	return true;
}

bool NarrowPhaseCollisionSystem::CapsuleAndCapsule(const ECS::EntityID capOneID, const ECS::EntityID capTwoID,
												   Math::RVec3 &outNormal, Math::RVec3 &outPoint,
												   Math::real &outPenetration) const {
	const Scene::Components::Transform &transformOne = *ref_eM->GetTComponent<Scene::Components::Transform>(capOneID);
	const Scene::Components::Transform &transformTwo = *ref_eM->GetTComponent<Scene::Components::Transform>(capTwoID);
	const Components::CapsuleCollider  &capsuleOne	 = *ref_eM->GetTComponent<Components::CapsuleCollider>(capOneID);
	const Components::CapsuleCollider  &capsuleTwo	 = *ref_eM->GetTComponent<Components::CapsuleCollider>(capTwoID);

	// 1. Calculate absolute world centers for both capsules
	Math::RVec3 worldCenterOne =
		transformOne.position + (transformOne.orientation * (capsuleOne.localOffset * transformOne.scale));
	Math::RVec3 worldCenterTwo =
		transformTwo.position + (transformTwo.orientation * (capsuleTwo.localOffset * transformTwo.scale));

	// Calculate the Up-Axes (The direction the capsules are pointing)
	Math::RVec3 upAxisOne = transformOne.orientation * Math::RVec3(0, 1, 0);
	Math::RVec3 upAxisTwo = transformTwo.orientation * Math::RVec3(0, 1, 0);

	// 2. Extract the internal line segments (spines) of both capsules
	Math::RVec3 segmentBottomOne = worldCenterOne - upAxisOne * capsuleOne.worldHalfHeight;
	Math::RVec3 segmentTopOne	 = worldCenterOne + upAxisOne * capsuleOne.worldHalfHeight;
	Math::RVec3 segmentBottomTwo = worldCenterTwo - upAxisTwo * capsuleTwo.worldHalfHeight;
	Math::RVec3 segmentTopTwo	 = worldCenterTwo + upAxisTwo * capsuleTwo.worldHalfHeight;

	// 3. Find the exact closest points between the two floating spines
	Math::RVec3 closestPointOne;
	Math::RVec3 closestPointTwo;
	ClosestPointsBetweenSegments(segmentBottomOne, segmentTopOne, segmentBottomTwo, segmentTopTwo, closestPointOne,
	                             closestPointTwo);

	// 4. Collision Check: Treat the closest points like two Spheres!
	Math::RVec3 vectorBetweenPoints = closestPointOne - closestPointTwo;
	Math::real	distanceSquared		= Math::Dot(vectorBetweenPoints, vectorBetweenPoints);
	Math::real	radiusSum			= capsuleOne.worldRadius + capsuleTwo.worldRadius;

	// Early exit: If the distance between the closest points is greater than their combined radii, they don't touch.
	if (distanceSquared > radiusSum * radiusSum) return false;

	// 5. Build Final Contact Data for the Physics Solver
	Math::real distance = Math::Sqrt(distanceSquared);

	// Normal collision (shallow penetration)
	if (distance > Math::REpsilon) {
		// Normal points from Capsule Two TO Capsule One
		outNormal	   = vectorBetweenPoints * (1.0f / distance);
		outPenetration = radiusSum - distance;
	} else {
		// Extreme Edge Case: The two spines perfectly overlap in space (distance is 0)
		// Push them apart using a default upward normal to prevent the engine from crashing
		outNormal	   = Math::RVec3(0, 1, 0);
		outPenetration = radiusSum;
	}

	// The contact point is on the surface of Capsule Two
	outPoint = closestPointTwo + outNormal * capsuleTwo.worldRadius;

	return true;
}

bool NarrowPhaseCollisionSystem::CapsuleAndBox(const ECS::EntityID capID, const ECS::EntityID boxID,
											   Math::RVec3 &outNormal, Math::RVec3 &outPoint,
											   Math::real &outPenetration) const {
	const Scene::Components::Transform &capsuleTransform = *ref_eM->GetTComponent<Scene::Components::Transform>(capID);
	const Scene::Components::Transform &boxTransform	 = *ref_eM->GetTComponent<Scene::Components::Transform>(boxID);
	const Components::CapsuleCollider  &capsuleCollider	 = *ref_eM->GetTComponent<Components::CapsuleCollider>(capID);
	const Components::BoxCollider	   &boxCollider		 = *ref_eM->GetTComponent<Components::BoxCollider>(boxID);

	// Calculate absolute world centers
	Math::RVec3 capsuleCenter = capsuleTransform.position +
								(capsuleTransform.orientation * (capsuleCollider.localOffset * capsuleTransform.scale));
	Math::RVec3 boxCenter =
		boxTransform.position + (boxTransform.orientation * (boxCollider.localOffset * boxTransform.scale));

	// Extract the Capsule's internal line segment (spine) in world space
	Math::RVec3 capsuleUpAxis		 = capsuleTransform.orientation * Math::RVec3(0, 1, 0);
	Math::RVec3 capsuleSegmentBottom = capsuleCenter - capsuleUpAxis * capsuleCollider.worldHalfHeight;
	Math::RVec3 capsuleSegmentTop	 = capsuleCenter + capsuleUpAxis * capsuleCollider.worldHalfHeight;

	// 1. Convert Capsule Segment to the Box's Local Space
	// This allows us to treat the Box as an unrotated, simple AABB (Axis-Aligned Bounding Box) at the origin (0,0,0).
	Math::RQuat boxInverseRotation = Math::Conjugate(boxTransform.orientation);
	Math::RVec3 localCapsuleBottom = boxInverseRotation * (capsuleSegmentBottom - boxCenter);
	Math::RVec3 localCapsuleTop	   = boxInverseRotation * (capsuleSegmentTop - boxCenter);

	// 2. Alternating Projection Solver
	// Find the absolute closest points between the Capsule's line segment and the Box.
	Math::RVec3 closestLocalCapsulePoint = (localCapsuleBottom + localCapsuleTop) * 0.5f;
	// Start at the middle of the segment
	Math::RVec3 closestLocalBoxPoint;

	// 3 iterations are geometrically and mathematically proven to converge on the true closest points
	// between an AABB and a line segment.
	for (int i = 0; i < 3; ++i) {
		// Project the capsule's point onto the Box (Clamp to AABB extents)
		closestLocalBoxPoint =
			Math::Clamp(closestLocalCapsulePoint, -boxCollider.worldHalfExtents, boxCollider.worldHalfExtents);
		// Project the box's point back onto the Capsule's line segment
		closestLocalCapsulePoint = ClosestPointOnSegment(localCapsuleBottom, localCapsuleTop, closestLocalBoxPoint);
	}

	// 3. Check Distance (Treating the closest point on the segment as a Sphere!)
	Math::RVec3 localVectorBetweenPoints = closestLocalCapsulePoint - closestLocalBoxPoint;
	Math::real	distanceSquared			 = Math::Dot(localVectorBetweenPoints, localVectorBetweenPoints);

	// Early exit: If the distance is greater than the capsule's radius, they are not touching.
	if (distanceSquared > capsuleCollider.worldRadius * capsuleCollider.worldRadius) return false;

	Math::real distance = Math::Sqrt(distanceSquared);

	// 4. Convert the closest points back to World Space
	Math::RVec3 closestWorldBoxPoint	 = boxCenter + (boxTransform.orientation * closestLocalBoxPoint);
	Math::RVec3 closestWorldCapsulePoint = boxCenter + (boxTransform.orientation * closestLocalCapsulePoint);

	// 5. Build Final Contact Data for the Physics Solver
	if (distance > Math::REpsilon) {
		// Normal collision: push direction is from the Box point to the Capsule point
		outNormal	   = (closestWorldCapsulePoint - closestWorldBoxPoint) * (1.0f / distance);
		outPenetration = capsuleCollider.worldRadius - distance;
	} else {
		// Edge Case: The capsule spine is perfectly inside a point on the box (distance is 0)
		// Push straight up to avoid a divide-by-zero engine crash
		outNormal	   = Math::RVec3(0, 1, 0);
		outPenetration = capsuleCollider.worldRadius;
	}

	outPoint = closestWorldBoxPoint;  // The contact point is exactly on the Box's surface
	return true;
}

bool NarrowPhaseCollisionSystem::BoxAndSphere(const ECS::EntityID boxID, const ECS::EntityID sphereID,
											  Math::RVec3 &outNormal, Math::RVec3 &outPoint,
											  Math::real &outPenetration) const {
	const Scene::Components::Transform &boxTf	 = *ref_eM->GetTComponent<Scene::Components::Transform>(boxID);
	const Scene::Components::Transform &sphereTf = *ref_eM->GetTComponent<Scene::Components::Transform>(sphereID);

	const Components::BoxCollider	 &boxCol	= *ref_eM->GetTComponent<Components::BoxCollider>(boxID);
	const Components::SphereCollider &sphereCol = *ref_eM->GetTComponent<Components::SphereCollider>(sphereID);

	Math::RVec3 boxWorldCenter = boxTf.position + (boxTf.orientation * (boxCol.localOffset * boxTf.scale));
	Math::RVec3 sphereWorldCenter =
		sphereTf.position + (sphereTf.orientation * (sphereCol.localOffset * sphereTf.scale));

	Math::RVec3 relCenter = Math::Conjugate(boxTf.orientation) * (sphereWorldCenter - boxWorldCenter);

	if (Math::Abs(relCenter.x) - sphereCol.worldRadius > boxCol.worldHalfExtents.x ||
		Math::Abs(relCenter.y) - sphereCol.worldRadius > boxCol.worldHalfExtents.y ||
		Math::Abs(relCenter.z) - sphereCol.worldRadius > boxCol.worldHalfExtents.z) {
		return false;
	}

	Math::RVec3 closestPt = Math::Clamp(relCenter, -boxCol.worldHalfExtents, boxCol.worldHalfExtents);

	Math::RVec3 difference = closestPt - relCenter;
	Math::real	distSq	   = Math::Dot(difference, difference);

	if (distSq > sphereCol.worldRadius * sphereCol.worldRadius) return false;

	Math::RVec3 closestPtWorld = (boxTf.orientation * closestPt) + boxWorldCenter;

	Math::RVec3 normal = closestPtWorld - sphereWorldCenter;

	outNormal	   = Math::Normalize(normal);
	outPoint	   = closestPtWorld;
	outPenetration = sphereCol.worldRadius - Math::Sqrt(distSq);
	return true;
}

bool NarrowPhaseCollisionSystem::BoxAndBox(const ECS::EntityID entityOne, const ECS::EntityID entityTwo,
										   Math::RVec3 &outNormal, Math::RVec3 &outPoint, Math::real &outPenetration) {
	const Components::BoxCollider	   &colOne = *ref_eM->GetTComponent<Components::BoxCollider>(entityOne);
	const Components::BoxCollider	   &colTwo = *ref_eM->GetTComponent<Components::BoxCollider>(entityTwo);
	const Scene::Components::Transform &tfOne  = *ref_eM->GetTComponent<Scene::Components::Transform>(entityOne);
	const Scene::Components::Transform &tfTwo  = *ref_eM->GetTComponent<Scene::Components::Transform>(entityTwo);

	// 1. Calculate world positions, incorporating local offset
	Math::RVec3 posOne = tfOne.position + (tfOne.orientation * (colOne.localOffset * tfOne.scale));
	Math::RVec3 posTwo = tfTwo.position + (tfTwo.orientation * (colTwo.localOffset * tfTwo.scale));

	Math::RVec3 toCenter = posTwo - posOne;

	// 2. Get the local axes for both boxes in world space
	Math::RVec3 axesOne[3] = {tfOne.orientation * Math::RVec3(1, 0, 0), tfOne.orientation * Math::RVec3(0, 1, 0),
							  tfOne.orientation * Math::RVec3(0, 0, 1)};

	Math::RVec3 axesTwo[3] = {tfTwo.orientation * Math::RVec3(1, 0, 0), tfTwo.orientation * Math::RVec3(0, 1, 0),
							  tfTwo.orientation * Math::RVec3(0, 0, 1)};

	// We start assuming there is no contact
	Math::real bestPenetration = std::numeric_limits<Math::real>::max();
	uint32_t   bestAxisIndex   = 0xffffff;

	auto tryAxis = [&](const Math::RVec3 &axis, const uint32_t index) -> bool {
		// Skip nearly parallel axes generated from cross products to prevent divide-by-zero
		if (Math::Dot(axis, axis) < Math::REpsilon) return true;

		const Math::RVec3 normalizedAxis = Math::Normalize(axis);

		const Math::real penetration =
			PenetrationOnAxis(colOne, tfOne.orientation, colTwo, tfTwo.orientation, normalizedAxis, toCenter);

		// If penetration is negative, we found a separating axis. No collision.
		if (penetration < 0) return false;

		// Keep track of the axis with the smallest penetration
		if (penetration < bestPenetration) {
			bestPenetration = penetration;
			bestAxisIndex	= index;
		}
		return true;
	};

	// 4. Test the 15 axes
	// Box 1's faces
	if (!tryAxis(axesOne[0], 0)) return false;
	if (!tryAxis(axesOne[1], 1)) return false;
	if (!tryAxis(axesOne[2], 2)) return false;

	// Box 2's faces
	if (!tryAxis(axesTwo[0], 3)) return false;
	if (!tryAxis(axesTwo[1], 4)) return false;
	if (!tryAxis(axesTwo[2], 5)) return false;

	// Store best single axis before cross products
	uint32_t bestSingleAxis = bestAxisIndex;

	// Edge-edge cross product axes
	// Adjust Math::Cross to your library's cross product function
	if (!tryAxis(Math::Cross(axesOne[0], axesTwo[0]), 6)) return false;
	if (!tryAxis(Math::Cross(axesOne[0], axesTwo[1]), 7)) return false;
	if (!tryAxis(Math::Cross(axesOne[0], axesTwo[2]), 8)) return false;
	if (!tryAxis(Math::Cross(axesOne[1], axesTwo[0]), 9)) return false;
	if (!tryAxis(Math::Cross(axesOne[1], axesTwo[1]), 10)) return false;
	if (!tryAxis(Math::Cross(axesOne[1], axesTwo[2]), 11)) return false;
	if (!tryAxis(Math::Cross(axesOne[2], axesTwo[0]), 12)) return false;
	if (!tryAxis(Math::Cross(axesOne[2], axesTwo[1]), 13)) return false;
	if (!tryAxis(Math::Cross(axesOne[2], axesTwo[2]), 14)) return false;

	// Ensure we have a valid result
	if (bestAxisIndex == 0xffffff) return false;
	outPenetration = bestPenetration;

	if (bestAxisIndex < 3) {
		// Vertex of Box 2 on Face of Box 1
		FillPointFaceBoxBox(tfOne, tfTwo, colTwo, toCenter, bestAxisIndex, outNormal, outPoint);
		return true;
	} else if (bestAxisIndex < 6) {
		// Vertex of Box 1 on Face of Box 2
		// We swap the parameters around, and reverse the toCenter vector
		FillPointFaceBoxBox(tfTwo, tfOne, colOne, toCenter * -1.0f, bestAxisIndex - 3, outNormal, outPoint);
		// FillPointFaceBoxBox always generates a normal from the Vertex-Box to the Face-Box.
		// Due to the swapped parameters above, the generated normal points from Box 1 to Box 2.
		// Our ContactResolver strictly expects normals to point from Box 2 to Box 1, so we must flip it.
		outNormal *= -1.0f;
		return true;
	} else {
		// Edge-Edge contact
		bestAxisIndex -= 6;
		uint32_t oneAxisIndex = bestAxisIndex / 3;
		uint32_t twoAxisIndex = bestAxisIndex % 3;

		Math::RVec3 oneAxis = axesOne[oneAxisIndex];
		Math::RVec3 twoAxis = axesTwo[twoAxisIndex];

		Math::RVec3 axis = Math::Normalize(Math::Cross(oneAxis, twoAxis));

		// The axis should point from box one to box two.
		if (Math::Dot(axis, toCenter) > 0) axis *= -1.0f;

		// Find out which of the 4 edges parallel to the axis we are colliding with
		Math::RVec3 ptOnOneEdge = colOne.worldHalfExtents;
		Math::RVec3 ptOnTwoEdge = colTwo.worldHalfExtents;

		for (uint32_t i = 0; i < 3; i++) {
			if (i == oneAxisIndex)
				ptOnOneEdge[i] = 0;
			else if (Math::Dot(axesOne[i], axis) > 0)
				ptOnOneEdge[i] = -ptOnOneEdge[i];

			if (i == twoAxisIndex)
				ptOnTwoEdge[i] = 0;
			else if (Math::Dot(axesTwo[i], axis) < 0)
				ptOnTwoEdge[i] = -ptOnTwoEdge[i];
		}

		// Move into world coordinates
		ptOnOneEdge = tfOne.position + (tfOne.orientation * ((colOne.localOffset * tfOne.scale) + ptOnOneEdge));
		ptOnTwoEdge = tfTwo.position + (tfTwo.orientation * ((colTwo.localOffset * tfTwo.scale) + ptOnTwoEdge));

		// Find the point of closest approach
		Math::RVec3 vertex = ContactPoint(ptOnOneEdge, oneAxis, colOne.worldHalfExtents[oneAxisIndex], ptOnTwoEdge,
										  twoAxis, colTwo.worldHalfExtents[twoAxisIndex], bestSingleAxis > 2);

		outNormal = axis;
		outPoint  = vertex;
		return true;
	}
}

void NarrowPhaseCollisionSystem::FillPointFaceBoxBox(const Scene::Components::Transform &tfOne,
													 const Scene::Components::Transform &tfTwo,
													 const Components::BoxCollider &colTwo, const Math::RVec3 &toCenter,
													 const uint32_t best, Math::RVec3 &outNormal,
													 Math::RVec3 &outPoint) const {
	// We know which axis the collision is on (i.e. 'best'),
	// but we need to work out which of the two faces on this axis.
	// 1. Get Box One's local axes in world space
	const Math::RVec3 axesOne[3] = {tfOne.orientation * Math::RVec3(1, 0, 0), tfOne.orientation * Math::RVec3(0, 1, 0),
									tfOne.orientation * Math::RVec3(0, 0, 1)};

	Math::RVec3 normal = axesOne[best];

	// If the normal is pointing in the same direction as the vector
	// between the centers, we need to flip it.
	if (Math::Dot(normal, toCenter) > 0) normal *= -1.0f;

	// 2. Work out which vertex of box two we're colliding with
	Math::RVec3 vertex = colTwo.worldHalfExtents;

	// Get Box Two's local axes in world space
	const Math::RVec3 axesTwo[3] = {tfTwo.orientation * Math::RVec3(1, 0, 0), tfTwo.orientation * Math::RVec3(0, 1, 0),
									tfTwo.orientation * Math::RVec3(0, 0, 1)};

	// Project the normal against box two's axes to find the deepest point
	if (Math::Dot(axesTwo[0], normal) < 0) vertex.x = -vertex.x;
	if (Math::Dot(axesTwo[1], normal) < 0) vertex.y = -vertex.y;
	if (Math::Dot(axesTwo[2], normal) < 0) vertex.z = -vertex.z;

	// Transform the local vertex to world space, incorporating the local offset
	const Math::RVec3 worldContactPos =
		tfTwo.position + (tfTwo.orientation * ((colTwo.localOffset * tfTwo.scale) + vertex));

	// 3. Fill the out values
	outNormal = normal;
	outPoint  = worldContactPos;
}

Math::RVec3 NarrowPhaseCollisionSystem::ContactPoint(const Math::RVec3 &pOne, const Math::RVec3 &dOne,
													 const Math::real oneSize, const Math::RVec3 &pTwo,
													 const Math::RVec3 &dTwo, const Math::real twoSize,
													 const bool useOne) {
	const Math::RVec3 toSt = pOne - pTwo;

	// Use dot products to find square magnitudes
	const Math::real smOne	  = Math::Dot(dOne, dOne);
	const Math::real smTwo	  = Math::Dot(dTwo, dTwo);
	const Math::real dpOneTwo = Math::Dot(dTwo, dOne);

	const Math::real dpStaOne = Math::Dot(dOne, toSt);
	const Math::real dpStaTwo = Math::Dot(dTwo, toSt);

	const Math::real denom = smOne * smTwo - dpOneTwo * dpOneTwo;

	// Zero denominator indicates parallel lines
	if (Math::Abs(denom) < Math::REpsilon) return useOne ? pOne : pTwo;

	const Math::real mua = (dpOneTwo * dpStaTwo - smTwo * dpStaOne) / denom;
	const Math::real mub = (smOne * dpStaTwo - dpOneTwo * dpStaOne) / denom;

	// If either of the edges has the nearest point out of bounds, then the
	// edges aren't crossed, we have an edge-face contact. Our point is on
	// the edge, which we know from the useOne parameter.
	if (mua > oneSize || mua < -oneSize || mub > twoSize || mub < -twoSize) {
		return useOne ? pOne : pTwo;
	} else {
		const Math::RVec3 cOne = pOne + dOne * mua;
		const Math::RVec3 cTwo = pTwo + dTwo * mub;

		return cOne * 0.5f + cTwo * 0.5f;
	}
}

Math::real NarrowPhaseCollisionSystem::PenetrationOnAxis(const Components::BoxCollider &colOne,
														 const Math::RQuat			   &orientOne,
														 const Components::BoxCollider &colTwo,
														 const Math::RQuat &orientTwo, const Math::RVec3 &axis,
														 const Math::RVec3 &toCenter) {
	const Math::real projectedOne = TransformToAxis(colOne.worldHalfExtents, orientOne, axis);
	const Math::real projectedTwo = TransformToAxis(colTwo.worldHalfExtents, orientTwo, axis);

	const Math::real distance = Math::Abs(Math::Dot(toCenter, axis));

	// Return the overlap (i.e., positive indicates
	// overlap, negative indicates separation).
	return projectedOne + projectedTwo - distance;
}

Math::real NarrowPhaseCollisionSystem::TransformToAxis(const Math::RVec3 &halfExtents, const Math::RQuat &orientation,
													   const Math::RVec3 &axis) {
	const Math::RVec3 xAxis = orientation * Math::RVec3(1, 0, 0);
	const Math::RVec3 yAxis = orientation * Math::RVec3(0, 1, 0);
	const Math::RVec3 zAxis = orientation * Math::RVec3(0, 0, 1);

	return halfExtents.x * Math::Abs(Math::Dot(xAxis, axis)) + halfExtents.y * Math::Abs(Math::Dot(yAxis, axis)) +
		   halfExtents.z * Math::Abs(Math::Dot(zAxis, axis));
}

void NarrowPhaseCollisionSystem::ExtractPoints(const Components::SphereCollider	  &col,
											   const Scene::Components::Transform &tf, PointCloud &cloud) {
	const Math::RVec3 center = tf.position + (tf.orientation * (col.localOffset * tf.scale));

	cloud.points[0] = center;
	cloud.radii[0]	= col.worldRadius;
	cloud.count		= 1;
}

void NarrowPhaseCollisionSystem::ExtractPoints(const Components::CapsuleCollider  &col,
											   const Scene::Components::Transform &tf, PointCloud &cloud) {
	const Math::RVec3 center = tf.position + (tf.orientation * (col.localOffset * tf.scale));
	const Math::RVec3 upAxis = tf.orientation * Math::RVec3(0, 1, 0);
	const Math::RVec3 offset = upAxis * col.worldHalfHeight;

	cloud.points[0] = center + offset;
	cloud.points[1] = center - offset;
	cloud.radii[0]	= col.worldRadius;
	cloud.radii[1]	= col.worldRadius;
	cloud.count		= 2;
}

void NarrowPhaseCollisionSystem::ExtractPoints(const Components::BoxCollider	  &col,
											   const Scene::Components::Transform &tf, PointCloud &cloud) {
	const Math::RVec3 center = tf.position + (tf.orientation * (col.localOffset * tf.scale));
	cloud.count		   = 0;

	for (int x = -1; x <= 1; x += 2) {
		for (int y = -1; y <= 1; y += 2) {
			for (int z = -1; z <= 1; z += 2) {
				Math::RVec3 corner(x * col.worldHalfExtents.x, y * col.worldHalfExtents.y, z * col.worldHalfExtents.z);
				cloud.points[cloud.count] = center + (tf.orientation * corner);
				cloud.radii[cloud.count]  = 0.0f;
				cloud.count++;
			}
		}
	}
}

void NarrowPhaseCollisionSystem::ExtractPoints(const Components::CylinderCollider &col,
											   const Scene::Components::Transform &tf, PointCloud &cloud) {
	const Math::RVec3 center = tf.position + (tf.orientation * (col.localOffset * tf.scale));
	cloud.count		   = 0;

	for (int x = -1; x <= 1; x += 2) {
		for (int y = -1; y <= 1; y += 2) {
			for (int z = -1; z <= 1; z += 2) {
				Math::RVec3 corner(x * col.worldRadius, y * col.worldHalfHeight, z * col.worldRadius);
				cloud.points[cloud.count] = center + (tf.orientation * corner);
				cloud.radii[cloud.count]  = 0.0f;
				cloud.count++;
			}
		}
	}
}

bool NarrowPhaseCollisionSystem::TestPoint(const Components::SphereCollider	  &col,
										   const Scene::Components::Transform &tf, const Math::RVec3 &pt, Math::real r,
										   Math::RVec3 &n, Math::RVec3 &outPt, Math::real &pen) {
	const Math::RVec3 center = tf.position + (tf.orientation * (col.localOffset * tf.scale));
	const Math::RVec3 diff   = pt - center;
	const Math::real	dist   = Math::Length(diff);

	if (dist + r > col.worldRadius) {
		n	  = (dist > Math::REpsilon) ? diff * (static_cast<Math::real>(-1.0) / dist) : Math::RVec3(0, -1, 0);
		pen	  = (dist + r) - col.worldRadius;
		outPt = pt - n * r;
		return true;
	}
	return false;
}

bool NarrowPhaseCollisionSystem::TestPoint(const Components::BoxCollider &col, const Scene::Components::Transform &tf,
										   const Math::RVec3 &pt, Math::real r, Math::RVec3 &n, Math::RVec3 &outPt,
										   Math::real &pen) {
	const Math::RVec3 center	= tf.position + (tf.orientation * (col.localOffset * tf.scale));
	const Math::RVec3 localPt = Math::Conjugate(tf.orientation) * (pt - center);

	Math::real	maxPen = static_cast<Math::real>(-1.0);
	Math::RVec3 localNormal(0, 0, 0);

	const Math::real penX = (Math::Abs(localPt.x) + r) - col.worldHalfExtents.x;
	if (penX > maxPen) {
		maxPen		= penX;
		localNormal = Math::RVec3((localPt.x > 0.0f) ? -1.0f : 1.0f, 0, 0);
	}

	const Math::real penY = (Math::Abs(localPt.y) + r) - col.worldHalfExtents.y;
	if (penY > maxPen) {
		maxPen		= penY;
		localNormal = Math::RVec3(0, (localPt.y > 0.0f) ? -1.0f : 1.0f, 0);
	}

	const Math::real penZ = (Math::Abs(localPt.z) + r) - col.worldHalfExtents.z;
	if (penZ > maxPen) {
		maxPen		= penZ;
		localNormal = Math::RVec3(0, 0, (localPt.z > 0.0f) ? -1.0f : 1.0f);
	}

	if (maxPen > 0.0f) {
		n	  = tf.orientation * localNormal;
		pen	  = maxPen;
		outPt = pt - n * r;
		return true;
	}
	return false;
}

bool NarrowPhaseCollisionSystem::TestPoint(const Components::CapsuleCollider  &col,
										   const Scene::Components::Transform &tf, const Math::RVec3 &pt, Math::real r,
										   Math::RVec3 &n, Math::RVec3 &outPt, Math::real &pen) {
	const Math::RVec3 center	= tf.position + (tf.orientation * (col.localOffset * tf.scale));
	const Math::RVec3 localPt = Math::Conjugate(tf.orientation) * (pt - center);

	const Math::real	spineY = Math::Clamp(localPt.y, -col.worldHalfHeight, col.worldHalfHeight);
	const Math::RVec3 spinePt(0, spineY, 0);

	const Math::RVec3 diff = localPt - spinePt;
	const Math::real	dist = Math::Length(diff);

	if (dist + r > col.worldRadius) {
		const Math::RVec3 localNormal =
			(dist > Math::REpsilon) ? diff * (static_cast<Math::real>(-1.0) / dist) : Math::RVec3(0, -1, 0);

		n	  = tf.orientation * localNormal;
		pen	  = (dist + r) - col.worldRadius;
		outPt = pt - n * r;
		return true;
	}
	return false;
}

bool NarrowPhaseCollisionSystem::TestPoint(const Components::CylinderCollider &col,
										   const Scene::Components::Transform &tf, const Math::RVec3 &pt, Math::real r,
										   Math::RVec3 &n, Math::RVec3 &outPt, Math::real &pen) {
	const Math::RVec3 center	= tf.position + (tf.orientation * (col.localOffset * tf.scale));
	const Math::RVec3 localPt = Math::Conjugate(tf.orientation) * (pt - center);

	Math::real  maxPen = -1.0;
	Math::RVec3 localNormal(0, 0, 0);

	const Math::real radialDist = Math::Sqrt(localPt.x * localPt.x + localPt.z * localPt.z);
	const Math::real radialPen  = (radialDist + r) - col.worldRadius;

	if (radialPen > maxPen) {
		maxPen		= radialPen;
		localNormal = (radialDist > Math::REpsilon) ? Math::RVec3(-localPt.x / radialDist, 0, -localPt.z / radialDist)
													: Math::RVec3(-1, 0, 0);
	}

	const Math::real axialPen = (Math::Abs(localPt.y) + r) - col.worldHalfHeight;
	if (axialPen > maxPen) {
		maxPen		= axialPen;
		localNormal = Math::RVec3(0, (localPt.y > 0.0f) ? -1.0f : 1.0f, 0);
	}

	if (maxPen > 0.0f) {
		n	  = tf.orientation * localNormal;
		pen	  = maxPen;
		outPt = pt - n * r;
		return true;
	}
	return false;
}
}  // namespace PE::Physics::Body::Systems