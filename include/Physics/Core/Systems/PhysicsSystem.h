#pragma once

#include "ECS/ISystem.h"
#include "ECS/ECSManager.h"
#include "Physics/Body/Systems/BodyPhysicsSystem.h"
#include "Physics/Particle/Types.h"
#include "Physics/Particle/Systems/ParticlePhysicsSystem.h"
#include "Scene/Systems/TransformSystem.h"

namespace PE::Physics::Particle::Components {
struct PointMass;
}

namespace PE::Physics::Core::Systems {
class PhysicsSystem : public ECS::ISystem {
public:
	explicit PhysicsSystem()						= default;
	PhysicsSystem(const PhysicsSystem &)			= delete;
	PhysicsSystem &operator=(const PhysicsSystem &) = delete;
	PhysicsSystem(PhysicsSystem &&)					= delete;
	PhysicsSystem &operator=(PhysicsSystem &&)		= delete;
	~PhysicsSystem() override						= default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::ECSManager *ecsManager, const PE::Core::EngineConfig &config,
						  Scene::Systems::TransformSystem &transformSystem);
	ERROR_CODE Shutdown() override;
	void	   OnUpdate(float dt) override;

	void TogglePhysics();
	void ResetSyncData();
	[[nodiscard]] Body::Systems::BodyPhysicsSystem &GetBodyPhysicsSystem() { return m_bodyPhysicsSystem; }
	[[nodiscard]] std::vector<Particle::TransformUpdateCommand> &GetParticleTransformQueue() {
		return m_particlePhysicsSystem.GetTransformQueue();
	}
	[[nodiscard]] std::vector<Body::TransformUpdateCommand> &GetBodyTransformQueue() {
		return m_bodyPhysicsSystem.GetTransformQueue();
	}
	[[nodiscard]] const std::vector<Particle::TransformUpdateCommand> &GetParticleTransformQueue() const {
		return m_particlePhysicsSystem.GetTransformQueue();
	}
	[[nodiscard]] const std::vector<Body::TransformUpdateCommand> &GetBodyTransformQueue() const {
		return m_bodyPhysicsSystem.GetTransformQueue();
	}

private:
	ECS::ECSManager					*ref_eM				 = nullptr;
	const PE::Core::EngineConfig	*ref_config			 = nullptr;
	Scene::Systems::TransformSystem *ref_transformSystem = nullptr;

	Particle::Systems::ParticlePhysicsSystem m_particlePhysicsSystem;
	Body::Systems::BodyPhysicsSystem		 m_bodyPhysicsSystem;

	bool		m_shouldPhysicsStop = false;
	float		m_accumulator		= 0.0f;
	const float m_fixedTimeStep		= 1.0f / 60.0f;
};
}  // namespace PE::Physics::Core::Systems