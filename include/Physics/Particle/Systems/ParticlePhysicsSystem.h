#pragma once
#include "Core/EngineConfig.h"
#include "ECS/ECSManager.h"
#include "Physics/Particle/ContactGenerator.h"
#include "Physics/Particle/ContactResolver.h"
#include "Physics/Particle/ForceGenerators.h"
#include "Physics/Particle/Types.h"
#include "Scene/Systems/TransformSystem.h"

namespace PE::Physics::Core::Systems {
class PhysicsSystem;
}

namespace PE::Physics::Particle::Systems {
class ParticlePhysicsSystem {
public:
	ParticlePhysicsSystem()											= default;
	ParticlePhysicsSystem(const ParticlePhysicsSystem &)			= delete;
	ParticlePhysicsSystem &operator=(const ParticlePhysicsSystem &) = delete;
	ParticlePhysicsSystem(ParticlePhysicsSystem &&)					= delete;
	ParticlePhysicsSystem &operator=(ParticlePhysicsSystem &&)		= delete;
	~ParticlePhysicsSystem()										= default;

	ERROR_CODE Initialize(ECS::ECSManager *ecsManager, const PE::Core::EngineConfig &config,
						  Scene::Systems::TransformSystem &transformSystem);
	ERROR_CODE Shutdown();
	void	   OnUpdate(float dt);

	void SyncWithTransform();
	void Integrate(float dt);

	[[nodiscard]] std::vector<TransformUpdateCommand> &GetTransformQueue() { return m_transformQueue; }
	[[nodiscard]] const std::vector<TransformUpdateCommand> &GetTransformQueue() const { return m_transformQueue; }
	[[nodiscard]] std::vector<Contact> &GetContacts() { return m_contacts; }

private:
	ECS::ECSManager					*ref_eM				 = nullptr;
	const PE::Core::EngineConfig	*ref_config			 = nullptr;
	Scene::Systems::TransformSystem *ref_transformSystem = nullptr;

	ForceGenerators	 m_forceGenerators;
	ContactGenerator m_contactGenerator;
	ContactResolver	 m_contactResolver;

	std::vector<Contact>				m_contacts;
	std::vector<TransformUpdateCommand> m_transformQueue;
};
}  // namespace PE::Physics::Particle::Systems