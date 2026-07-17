#include "Physics/Particle/Systems/ParticlePhysicsSystem.h"

#include "Physics/Particle/Components/PointMass.h"

namespace PE::Physics::Particle::Systems {

// TODO: Put somewhere like physics config!
constexpr Math::real	  Gravity			   = -9.81f;
constexpr static uint32_t ITERATION_MULTIPLIER = 2;

ERROR_CODE ParticlePhysicsSystem::Initialize(ECS::ECSManager *ecsManager, const PE::Core::EngineConfig &config,
											 Scene::Systems::TransformSystem &transformSystem) {
	ref_eM				= ecsManager;
	ref_config			= &config;
	ref_transformSystem = &transformSystem;

	m_transformQueue.reserve(config.maxEntityCount);
	m_contacts.reserve(config.maxEntityCount);

	ERROR_CODE result = ERROR_CODE::OK;

	return result;
}

ERROR_CODE ParticlePhysicsSystem::Shutdown() {
	ERROR_CODE result = ERROR_CODE::OK;
	return result;
}

void ParticlePhysicsSystem::OnUpdate(const float dt) {
	m_transformQueue.clear();
	m_forceGenerators.UpdateForces(ref_eM);
	Integrate(dt);

	// TODO: Is iteration count correct?
	if (const uint32_t contactCount = m_contactGenerator.GenerateContacts(ref_eM, this); contactCount > 0) {
		const uint32_t iterationCount = contactCount * ITERATION_MULTIPLIER;
		m_contactResolver.ResolveContacts(ref_eM, this, iterationCount, dt);
	}
	m_contacts.clear();
}

void ParticlePhysicsSystem::SyncWithTransform() {
	auto &transformArray = ref_eM->GetCompArr<Scene::Components::Transform>();
	auto &pmArray		 = ref_eM->GetCompArr<Components::PointMass>();

	for (auto const	   &updatedTransformEntityIDs = ref_transformSystem->GetUpdatedTransformEntityIDs();
		 const uint32_t entityID : updatedTransformEntityIDs) {
		if (pmArray.Has(entityID)) {
			Components::PointMass			   &pm = pmArray.Get(entityID);
			const Scene::Components::Transform &tf = transformArray.Get(entityID);

			pm.position = tf.position;
			pm.velocity = Math::RVec3(0.0f);
		}
	}
}

void ParticlePhysicsSystem::Integrate(const float dt) {
	auto							   &pmArray		= ref_eM->GetCompArr<Components::PointMass>();
	std::vector<Components::PointMass> &pointMasses = pmArray.Data();
	const std::vector<uint32_t>		   &entityIDs	= pmArray.Index();
	const uint32_t						pmCount		= pmArray.GetCount();

	// ==========================================
	// PHASE 1: INTEGRATION
	// ==========================================
	for (uint32_t i = 0; i < pmCount; ++i) {
		Components::PointMass &pm = pointMasses[i];

		// We don't integrate if mass is zero.
		if (pm.inverseMass <= 0.0f) continue;

		// Impose gravity
		const Math::RVec3 gravityAcc = {0, Gravity * pm.gravityScale, 0};
		const Math::RVec3 resultAcc	 = pm.acceleration + gravityAcc + (pm.forceAccum * pm.inverseMass);

		// Update linear velocity from acceleration
		pm.velocity += resultAcc * dt;

		// Impose drag
		pm.velocity *= 1.0 / (1.0f + (pm.linearDamping * dt));

		// Update linear position
		pm.position += pm.velocity * dt;

		// Clear frame-specific forces so they don't accumulate forever
		pm.ClearAccumulator();

		if (const Math::real speedSq = Math::Dot(pm.velocity, pm.velocity); speedSq > 0.000001f) {
			m_transformQueue.emplace_back(TransformUpdateCommand{entityIDs[i], pm.position});
		}
	}
}
}  // namespace PE::Physics::Particle::Systems