#pragma once

#include "ECS/ECSManager.h"
#include "Physics/Body/Components/AABB.h"
#include "Physics/Body/Components/BoxCollider.h"
#include "Physics/Body/Components/CapsuleCollider.h"
#include "Physics/Body/Components/CylinderCollider.h"
#include "Physics/Body/Components/SphereCollider.h"
#include "Scene/Systems/TransformSystem.h"

namespace PE::Physics::Body::Systems {
class AABBUpdateSystem : public ECS::ISystem {
public:
	explicit AABBUpdateSystem()							  = default;
	AABBUpdateSystem(const AABBUpdateSystem &)			  = delete;
	AABBUpdateSystem &operator=(const AABBUpdateSystem &) = delete;
	AABBUpdateSystem(AABBUpdateSystem &&)				  = delete;
	AABBUpdateSystem &operator=(AABBUpdateSystem &&)	  = delete;
	~AABBUpdateSystem() override;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::ECSManager *ecsManager,
						  Scene::Systems::TransformSystem *transformSystem,
						  Core::Systems::PhysicsSystem	  *physicsSystem);
	ERROR_CODE Shutdown() override;
	void	   OnUpdate(float dt) override;
	void	   UpdateAABB(Components::AABB &outAABB, Components::BoxCollider &boxCol, const Math::Mat44 &m);
	void	   UpdateAABB(Components::AABB &outAABB, Components::CapsuleCollider &capCol,
											const Math::Mat44 &m);
	void	   UpdateAABB(Components::AABB &outAABB, Components::CylinderCollider &cylinderCol,
											 const Math::Mat44 &m);
	void	   UpdateAABB(Components::AABB &outAABB, Components::SphereCollider &sphereCol,
										   const Math::Mat44 &m);

private:
	ECS::ECSManager					*ref_eM				 = nullptr;
	Scene::Systems::TransformSystem *ref_transformSystem = nullptr;
	Core::Systems::PhysicsSystem	*ref_physicsSystem	 = nullptr;
};
}  // namespace PE::Physics::Body::Systems