#pragma once
#include "BroadPhaseCollisionSystem.h"
#include "Core/EngineConfig.h"
#include "ECS/ECSManager.h"
#include "NarrowPhaseCollisionSystem.h"
#include "Physics/Body/Components/RigidBody.h"
#include "Physics/Body/ForceGenerators.h"
#include "Physics/Body/Types.h"
#include "PhysicsMaterialSystem.h"
#include "Scene/Systems/TransformSystem.h"
#include "Scene/Systems/WaypointAnimationSystem.h"

namespace PE::Physics::Core::Systems {
class PhysicsSystem;
}

namespace PE::Physics::Body::Systems {

class BodyPhysicsSystem {
public:
	explicit BodyPhysicsSystem()							= default;
	BodyPhysicsSystem(const BodyPhysicsSystem &)			= delete;
	BodyPhysicsSystem &operator=(const BodyPhysicsSystem &) = delete;

	BodyPhysicsSystem(BodyPhysicsSystem &&)			   = delete;
	BodyPhysicsSystem &operator=(BodyPhysicsSystem &&) = delete;
	~BodyPhysicsSystem()							   = default;

	ERROR_CODE Initialize(ECS::ECSManager *ecsManager, const PE::Core::EngineConfig &config,
						  Scene::Systems::TransformSystem &transformSystem);
	ERROR_CODE Shutdown();
	void	   OnUpdate(float dt);

	void                                                     SyncWithTransform() const;
	[[nodiscard]] PhysicsMaterialSystem	   &              GetPhysicsMaterialSystem() { return m_physicsMaterialSystem; }
	[[nodiscard]] Scene::Systems::WaypointAnimationSystem &  GetWaypointAnimationSystem() { return m_waypointAnimationSystem; }
	[[nodiscard]] std::vector<TransformUpdateCommand> &GetTransformQueue() { return m_transformQueue; }
	[[nodiscard]] const std::vector<TransformUpdateCommand> &GetTransformQueue() const { return m_transformQueue; }

private:
	void UpdateKinematicBodies(float dt) const;
	void Integrate(float dt) const;
	void IntegrateAngularPosition(Components::RigidBody &rb, float dt) const;
	void FillTransformUpdateQueue();

	ECS::ECSManager					*ref_eM				 = nullptr;
	const PE::Core::EngineConfig	*ref_config			 = nullptr;
	Scene::Systems::TransformSystem *ref_transformSystem = nullptr;

	BroadPhaseCollisionSystem               m_broadPhaseCollisionSystem;
	NarrowPhaseCollisionSystem              m_narrowPhaseCollisionSystem;
	PhysicsMaterialSystem                   m_physicsMaterialSystem;
	Scene::Systems::WaypointAnimationSystem m_waypointAnimationSystem;
	ForceGenerators                         m_forceGenerators;
	std::vector<TransformUpdateCommand>     m_transformQueue;
};
}  // namespace PE::Physics::Body::Systems
