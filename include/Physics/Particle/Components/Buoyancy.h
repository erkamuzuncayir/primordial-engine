#pragma once
#include "Math/Math.h"

namespace PE::Physics::Particle::Components {
struct Buoyancy {
	Buoyancy() = default;
	explicit Buoyancy(const Math::real maxDepth, const Math::real volume, const Math::real waterHeight,
					  const Math::real liquidDensity = 1000.0f)
		: maxDepth(maxDepth), volume(volume), waterHeight(waterHeight), liquidDensity(liquidDensity) {}

	// The maximum submersion depth of the object before it generates its maximum buoyancy force.
	Math::real maxDepth{1};

	// The volume of the object.
	Math::real volume{1};

	// The height of the water plane above y = 0. The plane will be parallel to the XZ plane.
	Math::real waterHeight{1};

	// The density of the liquid. Pure water has a density of 1000 kg per cubic meter.
	Math::real liquidDensity{1000};
};
}  // namespace PE::Physics::Particle::Components