#pragma once
#include "../Components/Spawner.h"
#include "ECS/ECSManager.h"
#include "Graphics/RenderTypes.h"
#include "Physics/Body/Components/RigidBody.h"
#include "Scene/Components/Transform.h"

namespace PE::Scene::Systems {
class SpawnSystem {
public:
	explicit SpawnSystem()						= default;
	SpawnSystem(const SpawnSystem &)			= delete;
	SpawnSystem &operator=(const SpawnSystem &) = delete;
	SpawnSystem(SpawnSystem &&)					= delete;
	SpawnSystem &operator=(SpawnSystem &&)		= delete;
	~SpawnSystem()								= default;

	void Initialize(ECS::ECSManager *ecsManager);
	void Shutdown();
	void OnUpdate(float dt);
	void Reset();

private:
	void								 ProcessSpawner(float dt);
	void								 SpawnSingleBurst(const Components::Spawner &spawner);
	void								 CreateSimulationEntity(const Components::Spawner &spawner);
	void								 CreateTagComponent(const std::string &name, ECS::EntityID entityID) const;
	Components::Transform				 CreateTransformComponent(const Components::ULocation &location);
	Physics::Body::Components::RigidBody CreateCommonRigidBody(const Components::Transform &tf,
															   const Math::Vec3Range	   &velRange,
															   const Math::Vec3Range &angVelRange, bool isGravityOn);
	void CreateColliderAndAABBComponent(ECS::EntityID entityID, Physics::Body::Components::ColliderShape shape) const;
	void CreateMeshRendererComponent(ECS::EntityID entityID, Graphics::MeshID meshID) const;

	ECS::ECSManager			  *ref_eM{nullptr};
	float					   m_timePassed{0};
	std::vector<ECS::EntityID> removeIndex;
};
}  // namespace PE::Scene::Systems