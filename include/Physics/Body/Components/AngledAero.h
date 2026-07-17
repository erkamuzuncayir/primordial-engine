#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {

struct AngledAero {
	// Holds the aerodynamic tensor for the surface in body space.
	Math::RMat33 tensor{0};

	// Holds the relative position of the aerodynamic surface in body coordinates.
	Math::RVec3 position{0};

	// Holds the orientation of the aerodynamic surface relative to the rigid body to which it is attached.
	Math::RQuat orientation{1.0, 0.0, 0.0, 0.0};
};
}  // namespace PE::Physics::Body::Components