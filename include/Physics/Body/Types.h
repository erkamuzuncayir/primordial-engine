#pragma once

#include "ECS/Entity.h"
#include "Math/Math.h"

namespace PE::Physics::Body {

// Assignment fields
using PhysicsMaterialID									= uint32_t;
constexpr PhysicsMaterialID INVALID_PHYSICS_MATERIAL_ID = UINT32_MAX;

struct MaterialInteraction {
	Math::real restitution;
	Math::real staticFriction;
	Math::real dynamicFriction;
};

struct TransformUpdateCommand {
	TransformUpdateCommand(const ECS::EntityID id, const Math::RVec3 pos, const Math::RQuat orientation)
		: id(id), newPosition(pos), newOrientation(orientation) {}
	ECS::EntityID id;
	Math::RVec3	  newPosition;
	Math::RQuat	  newOrientation;
};

struct RigidBodyContact {
	ECS::EntityID entityOne = ECS::INVALID_ENTITY_ID;
	ECS::EntityID entityTwo = ECS::INVALID_ENTITY_ID;

	Math::RVec3 point{};
	Math::RVec3 normal{};
	Math::real	penetration{0};
	Math::real	restitution{1.0};
	Math::real	staticFriction{0.2};
	Math::real	dynamicFriction{0.2};

	// A transform matrix that converts coordinates in the contact’s frame of reference to world coordinates.
	// The columns of this matrix form an orthonormal set of vectors.
	Math::RMat33 contactToWorld{};
	// Holds the closing velocity at the point of contact. This is set when the calculateInternals function is run.
	Math::RVec3 velocity{};
	// Holds the required change in velocity for this contact to be resolved.
	Math::real desiredDeltaVelocity{};
	// Holds the world-space position of the contact point relative to The center of each body.
	// This is set when the calculateInternals function is run.
	Math::RVec3 relativeContactPosition[2];
};

struct KinematicContact {
	ECS::EntityID entityOne = ECS::INVALID_ENTITY_ID;

	Math::RVec3 point{};
	Math::RVec3 normal{};
	Math::real	penetration{0};
	Math::real	restitution{1.0};
	Math::real	staticFriction{0.2};
	Math::real	dynamicFriction{0.2};

	// A transform matrix that converts coordinates in the contact’s frame of reference to world coordinates.
	// The columns of this matrix form an orthonormal set of vectors.
	Math::RMat33 contactToWorld{};
	// Holds the closing velocity at the point of contact. This is set when the calculateInternals function is run.
	Math::RVec3 velocity{};
	// Holds the required change in velocity for this contact to be resolved.
	Math::real desiredDeltaVelocity{};
	// Holds the world-space position of the contact point relative to the center of dynamic body.
	// This is set when the calculateInternals function is run.
	Math::RVec3 relativeContactPosition{};
	// Pre-calculated surface velocity at the contact point (avoids recalculation in the solver).
	Math::RVec3 kinematicSurfaceVelocity{};
};

struct StaticContact {
	ECS::EntityID entityOne = ECS::INVALID_ENTITY_ID;
	Math::RVec3	  point{};
	Math::RVec3	  normal{};
	Math::real	  penetration{0};
	Math::real	  restitution{1.0};
	Math::real	  staticFriction{0.5};
	Math::real	  dynamicFriction{0.5};

	// A transform matrix that converts coordinates in the contact’s frame of reference to world coordinates.
	// The columns of this matrix form an orthonormal set of vectors.
	Math::RMat33 contactToWorld{};
	// Holds the closing velocity at the point of contact. This is set when the calculateInternals function is run.
	Math::RVec3 velocity{};
	// Holds the required change in velocity for this contact to be resolved.
	Math::real desiredDeltaVelocity{};
	// Holds the world-space position of the contact point relative to the center of body.
	// This is set when the calculateInternals function is run.
	Math::RVec3 relativeContactPosition{};
};

struct PointCloud {
	Math::RVec3 points[8];
	Math::real	radii[8];
	size_t		count = 0;
};
}  // namespace PE::Physics::Body