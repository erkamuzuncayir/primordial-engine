#pragma once

#include "Math/Math.h"
#include <vector>

namespace PE::Scene::Components {
struct Waypoint {
	Math::RVec3 position;
	Math::RQuat orientation;
	Math::real	time;
};

struct WaypointAnimation {
	enum class EasingType : int8_t { Linear, SmoothStep };
	enum class PathMode : int8_t { Stop, Loop, Reverse };
	enum class Status : int8_t { NotStarted, GoingForward, ReturningBack, LoopingBack, Finished };
	Math::Vec3		startPos{};
	Math::Quat		startOrientation{};
	uint32_t		currWaypointIndex{0};
	Math::real		currWaypointPassedTime{0};
	Math::real		duration{0};
	EasingType		easing{};
	PathMode		pathMode{};
	Status status{};
	std::vector<Waypoint> waypoints;
};
}  // namespace PE::Scene::Components