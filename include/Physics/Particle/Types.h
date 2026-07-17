#pragma once
#include "ECS/Entity.h"
#include "Math/Math.h"

namespace PE::Physics::Particle {

struct TransformUpdateCommand {
	ECS::Entity id;
	Math::RVec3 newPosition;
};
}  // namespace PE::Physics::Particle