#include "Physics/Core/Systems/PhysicsSystem.h"

namespace PE::Physics::Core::Systems {
ERROR_CODE PhysicsSystem::Initialize(const ECS::ESystemStage stage, ECS::ECSManager *ecsManager,
									 const PE::Core::EngineConfig	 &config,
									 Scene::Systems::TransformSystem &transformSystem) {
	m_state	 = SystemState::Initializing;
	m_typeID = GetUniqueISystemTypeID<PhysicsSystem>();

	ref_eM				= ecsManager;
	ref_config			= &config;
	ref_transformSystem = &transformSystem;
	m_stage				= stage;

	ERROR_CODE result;
	PE_CHECK(result, ref_eM->RegisterSystem(this));
	PE_CHECK(result, m_bodyPhysicsSystem.Initialize(ecsManager, config, transformSystem));
	PE_CHECK(result, m_particlePhysicsSystem.Initialize(ecsManager, config, transformSystem));
	m_state = SystemState::Running;

	return result;
}

ERROR_CODE PhysicsSystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	ERROR_CODE result;
	PE_CHECK(result, m_particlePhysicsSystem.Shutdown());
	PE_CHECK(result, ref_eM->UnregisterSystem(this));
	m_stage	 = ECS::ESystemStage::Count;
	m_typeID = UINT32_MAX;
	m_state	 = SystemState::Uninitialized;

	return result;
}

void PhysicsSystem::OnUpdate(Math::real dt) {
	if (m_shouldPhysicsStop) return;

	m_particlePhysicsSystem.SyncWithTransform();
	m_bodyPhysicsSystem.SyncWithTransform();

	if (dt > 0.25f) dt = 0.25f;
	m_accumulator += dt;

	while (m_accumulator >= m_fixedTimeStep) {
		m_particlePhysicsSystem.OnUpdate(m_fixedTimeStep);
		m_bodyPhysicsSystem.OnUpdate(m_fixedTimeStep);
		m_accumulator -= m_fixedTimeStep;
	}
}

void PhysicsSystem::TogglePhysics() { m_shouldPhysicsStop = !m_shouldPhysicsStop; }

void PhysicsSystem::ResetSyncData() {
	GetParticleTransformQueue().clear();
	GetBodyTransformQueue().clear();
}
}  // namespace PE::Physics::Core::Systems