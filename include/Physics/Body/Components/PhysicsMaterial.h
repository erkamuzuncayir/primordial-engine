#pragma once
#include "Physics/Body/Types.h"

namespace PE::Physics::Body::Components {
struct PhysicsMaterial {
	PhysicsMaterialID id{INVALID_PHYSICS_MATERIAL_ID};
};
}  // namespace PE::Physics::Body::Components