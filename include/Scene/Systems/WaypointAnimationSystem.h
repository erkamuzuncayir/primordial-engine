#pragma once
#include "ECS/ECSManager.h"
#include "Scene/Components/WaypointAnimation.h"

namespace PE::Scene::Systems {

class WaypointAnimationSystem {
public:
	explicit WaypointAnimationSystem()									  = default;
	WaypointAnimationSystem(const WaypointAnimationSystem &)			  = delete;
	WaypointAnimationSystem &operator=(const WaypointAnimationSystem &) = delete;
	WaypointAnimationSystem(WaypointAnimationSystem &&)				  = delete;
	WaypointAnimationSystem &operator=(WaypointAnimationSystem &&)	  = delete;
	~WaypointAnimationSystem()											  = default;

	void Initialize(ECS::ECSManager *ecsManager);
	void OnUpdate(Math::real dt) const;

	void ProcessNotStartedPhase(ECS::EntityID entityID, Components::WaypointAnimation &animation) const;
	void ProcessGoingForwardPhase(ECS::EntityID entityID, Components::WaypointAnimation &animation,
								  Math::real    dt) const;
	void ProcessLoopingBackPhase(ECS::EntityID entityID, Components::WaypointAnimation &animation,
								 Math::real dt) const;
	void ProcessReturningBackPhase(ECS::EntityID entityID, Components::WaypointAnimation &animation,
								   Math::real dt) const;
	void ProcessFinishedPhase(ECS::EntityID entityID) const;

private:
	ECS::ECSManager						   *ref_eM{nullptr};
};
}  // namespace PE::Scene::Systems