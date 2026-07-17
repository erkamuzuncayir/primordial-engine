#include "Scene/Systems/WaypointAnimationSystem.h"

#include "Scene/Components/Transform.h"

namespace PE::Scene::Systems {
void WaypointAnimationSystem::Initialize(ECS::ECSManager *ecsManager) { ref_eM = ecsManager; }


void WaypointAnimationSystem::OnUpdate(const Math::real dt) const {
	auto &animationArr = ref_eM->GetCompArr<Components::WaypointAnimation>();
	for (int i = 0; i < animationArr.GetCount(); ++i) {
		const ECS::EntityID entityID  = animationArr.Index()[i];
		auto &              animation = animationArr.Data()[i];
		switch (animation.status) {
				using status = Components::WaypointAnimation::Status;
			case status::NotStarted: ProcessNotStartedPhase(entityID, animation);
				break;
			case status::GoingForward: ProcessGoingForwardPhase(entityID, animation, dt);
				break;
			case status::ReturningBack: ProcessReturningBackPhase(entityID, animation, dt);
				break;
			case status::LoopingBack: ProcessLoopingBackPhase(entityID, animation, dt);
				break;
			case status::Finished: ProcessFinishedPhase(entityID);
				break;
		}
	}
}

void WaypointAnimationSystem::ProcessNotStartedPhase(const ECS::EntityID            entityID,
                                                     Components::WaypointAnimation &animation) const {
	if (animation.waypoints.empty()) {
		PE_LOG_ERROR("Waypoint array is empty!");
		return;
	}
	auto &transform = *(ref_eM->GetTComponent<Components::Transform>(entityID));

	animation.currWaypointIndex      = 0;
	animation.startPos               = transform.position;
	animation.startOrientation       = transform.orientation;
	animation.currWaypointPassedTime = 0.0f;
	animation.status                 = Components::WaypointAnimation::Status::GoingForward;
}

void WaypointAnimationSystem::ProcessGoingForwardPhase(const ECS::EntityID            entityID,
                                                       Components::WaypointAnimation &animation,
                                                       const Math::real               dt) const {
	animation.currWaypointPassedTime += dt;
	const uint32_t lastIndex         = animation.waypoints.size() - 1;

	while (true) {
		if (animation.currWaypointIndex > lastIndex) {
			if (animation.pathMode == Components::WaypointAnimation::PathMode::Stop) {
				animation.status = Components::WaypointAnimation::Status::Finished;
				return;
			} else if (animation.pathMode == Components::WaypointAnimation::PathMode::Reverse) {
				animation.status            = Components::WaypointAnimation::Status::ReturningBack;
				animation.currWaypointIndex = lastIndex - 1;
				return;
			} else if (animation.pathMode == Components::WaypointAnimation::PathMode::Loop) {
				animation.status = Components::WaypointAnimation::Status::LoopingBack;
				return;
			}
		}

		const auto &targetWaypoint = animation.waypoints[animation.currWaypointIndex];
		Math::real  segmentTime    = 0.0f;

		if (animation.currWaypointIndex == 0) {
			segmentTime = targetWaypoint.time;
		} else {
			segmentTime = targetWaypoint.time - animation.waypoints[animation.currWaypointIndex - 1].time;
		}

		if (segmentTime <= 0.0f) {
			animation.startPos         = targetWaypoint.position;
			animation.startOrientation = targetWaypoint.orientation;
			animation.currWaypointIndex++;
			continue;
		}

		if (animation.currWaypointPassedTime >= segmentTime) {
			animation.currWaypointPassedTime -= segmentTime;
			animation.startPos               = targetWaypoint.position;
			animation.startOrientation       = targetWaypoint.orientation;
			animation.currWaypointIndex++;
		} else {
			Math::real alpha = animation.currWaypointPassedTime / segmentTime;

			if (animation.easing == Components::WaypointAnimation::EasingType::SmoothStep) {
				alpha = alpha * alpha * (3.0f - 2.0f * alpha);
			}

			const Math::Vec3 newPosition = Math::Lerp(animation.startPos, targetWaypoint.position, alpha);

			Math::Quat targetOrient = targetWaypoint.orientation;
			if (Math::Dot(animation.startOrientation, targetOrient) < 0.0f) {
				targetOrient = -targetOrient;
			}

			const Math::Quat newOrientation =
				Math::Normalize(Math::Lerp(animation.startOrientation, targetOrient, alpha));

			auto &transform       = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
			transform.position    = newPosition;
			transform.orientation = newOrientation;
			transform.state       = Components::Transform::TransformState::Dirty;
			break;
		}
	}
}

void WaypointAnimationSystem::ProcessLoopingBackPhase(const ECS::EntityID            entityID,
                                                      Components::WaypointAnimation &animation,
                                                      const Math::real               dt) const {
	animation.currWaypointPassedTime += dt;

	const uint32_t lastIndex      = animation.waypoints.size() - 1;
	const auto &   targetWaypoint = animation.waypoints[0];

	const Math::real segmentTime = animation.duration - animation.waypoints[lastIndex].time;

	if (segmentTime <= 0.0f) {
		animation.startPos         = targetWaypoint.position;
		animation.startOrientation = targetWaypoint.orientation;

		animation.currWaypointIndex = 1;
		animation.status            = Components::WaypointAnimation::Status::GoingForward;
		return;
	}

	if (animation.currWaypointPassedTime >= segmentTime) {
		animation.currWaypointPassedTime -= segmentTime;
		animation.startPos               = targetWaypoint.position;
		animation.startOrientation       = targetWaypoint.orientation;

		animation.currWaypointIndex = 1;
		animation.status            = Components::WaypointAnimation::Status::GoingForward;
	} else {
		Math::real alpha = animation.currWaypointPassedTime / segmentTime;

		if (animation.easing == Components::WaypointAnimation::EasingType::SmoothStep) {
			alpha = alpha * alpha * (3.0f - 2.0f * alpha);
		}

		const Math::Vec3 newPosition = Math::Lerp(animation.startPos, targetWaypoint.position, alpha);

		Math::Quat targetOrient = targetWaypoint.orientation;
		if (Math::Dot(animation.startOrientation, targetOrient) < 0.0f) {
			targetOrient = -targetOrient;
		}

		const Math::Quat newOrientation = Math::Normalize(Math::Lerp(animation.startOrientation, targetOrient, alpha));

		auto &transform       = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
		transform.position    = newPosition;
		transform.orientation = newOrientation;
		transform.state       = Components::Transform::TransformState::Dirty;
	}
}

void WaypointAnimationSystem::ProcessReturningBackPhase(const ECS::EntityID            entityID,
                                                        Components::WaypointAnimation &animation,
                                                        const Math::real               dt) const {
	animation.currWaypointPassedTime += dt;

	while (true) {
		const auto &targetWaypoint = animation.waypoints[animation.currWaypointIndex];

		const Math::real segmentTime = animation.waypoints[animation.currWaypointIndex + 1].time - targetWaypoint.time;

		if (segmentTime <= 0.0f) {
			animation.startPos         = targetWaypoint.position;
			animation.startOrientation = targetWaypoint.orientation;

			if (animation.currWaypointIndex == 0) {
				animation.currWaypointIndex = 1;
				animation.status            = Components::WaypointAnimation::Status::GoingForward;

				ProcessGoingForwardPhase(entityID, animation, 0.0f);
				return;
			}

			animation.currWaypointIndex--;
			continue;
		}

		if (animation.currWaypointPassedTime >= segmentTime) {
			animation.currWaypointPassedTime -= segmentTime;
			animation.startPos               = targetWaypoint.position;
			animation.startOrientation       = targetWaypoint.orientation;

			if (animation.currWaypointIndex == 0) {
				animation.currWaypointIndex = 1;
				animation.status            = Components::WaypointAnimation::Status::GoingForward;

				ProcessGoingForwardPhase(entityID, animation, 0.0f);
				return;
			}

			animation.currWaypointIndex--;
		} else {
			Math::real alpha = animation.currWaypointPassedTime / segmentTime;

			if (animation.easing == Components::WaypointAnimation::EasingType::SmoothStep) {
				alpha = alpha * alpha * (3.0f - 2.0f * alpha);
			}

			const Math::Vec3 newPosition = Math::Lerp(animation.startPos, targetWaypoint.position, alpha);

			Math::Quat targetOrient = targetWaypoint.orientation;
			if (Math::Dot(animation.startOrientation, targetOrient) < 0.0f) {
				targetOrient = -targetOrient;
			}

			const Math::Quat newOrientation =
				Math::Normalize(Math::Lerp(animation.startOrientation, targetOrient, alpha));

			auto &transform       = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
			transform.position    = newPosition;
			transform.orientation = newOrientation;
			transform.state       = Components::Transform::TransformState::Dirty;
			break;
		}
	}
}

void WaypointAnimationSystem::ProcessFinishedPhase(const ECS::EntityID entityID) const {
	ref_eM->RemoveComponent<Components::WaypointAnimation>(entityID);
}
}