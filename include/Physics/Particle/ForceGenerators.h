#pragma once
#include "ECS/ECSManager.h"

namespace PE::Physics::Particle {
class ForceGenerators {
public:
	void UpdateForces(ECS::ECSManager *eM);
	// A force generator that applies a spring force both of the sides but only when extended.
	void UpdateAnchoredBungeeForces(ECS::ECSManager *eM);
	// A force generator that applies a spring force just one side of the spring.
	void UpdateAnchoredSpringForces(ECS::ECSManager *eM);
	// A force generator that applies a buoyancy force for a plane of liquid parallel to XZ plane.
	void UpdateBuoyancyForces(ECS::ECSManager *eM);
	// A force generator that applies a buoyancy force for a plane of liquid parallel to XZ plane.
	void UpdateDragForces(ECS::ECSManager *eM);
	// A force generator that applies a spring force both of the sides.
	void UpdateSpringForces(ECS::ECSManager *eM);
};
}  // namespace PE::Physics::Particle