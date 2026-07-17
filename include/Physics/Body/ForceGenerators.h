#pragma once
#include "ECS/ECSManager.h"
#include "Math/Math.h"

namespace PE::Physics::Body {
class ForceGenerators {
public:
	void UpdateForces(ECS::ECSManager *eM);
private:
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
	// A force generator that applies an aerodynamic force.
	void UpdateAeroForces(ECS::ECSManager *eM);
	//  A force generator with a control aerodynamic surface. This requires three inertia tensors, for the two extremes
	//  and **resting** position of the control surface.
	void UpdateAeroControlForces(ECS::ECSManager *eM);
	// A force generator with an aerodynamic surface that can be reoriented relative to its rigid body.
	void UpdateAngledAeroForces(ECS::ECSManager *eM, const Math::RVec3 &windSpeed);

	// For Aero
	// Holds a pointer to a vector containing the wind speed of the environment.
	// TODO: Move this to somewhere, maybe physics config, general physics environment settings or another component?
	Math::RVec3 windSpeed;
};
}  // namespace PE::Physics::Body