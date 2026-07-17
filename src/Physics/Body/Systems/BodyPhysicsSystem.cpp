#include "Physics/Body/Systems/BodyPhysicsSystem.h"

#include "Physics/Body/Components/RigidBody.h"
#include "Physics/Body/Utilities.h"

namespace PE::Physics::Body::Systems {

static const Math::RVec3 GravityAcceleration{0.0, -9.81, 0.0};

ERROR_CODE BodyPhysicsSystem::Initialize(ECS::ECSManager *ecsManager, const PE::Core::EngineConfig &config,
										 Scene::Systems::TransformSystem &transformSystem) {
	ref_eM				= ecsManager;
	ref_config			= &config;
	ref_transformSystem = &transformSystem;

	ref_eM = ecsManager;
	m_transformQueue.reserve(config.maxEntityCount);
	m_broadPhaseCollisionSystem.Initialize(ref_eM);
	m_narrowPhaseCollisionSystem.Initialize(ref_eM, config, &m_physicsMaterialSystem);
	m_waypointAnimationSystem.Initialize(ref_eM);
	ERROR_CODE result = ERROR_CODE::OK;

	return result;
}

ERROR_CODE BodyPhysicsSystem::Shutdown() {
	ERROR_CODE result = ERROR_CODE::OK;
	return result;
}

void BodyPhysicsSystem::OnUpdate(const float dt) {
	m_waypointAnimationSystem.OnUpdate(dt);
	if (dt > 0.0f) UpdateKinematicBodies(dt);
	m_transformQueue.clear();
	m_forceGenerators.UpdateForces(ref_eM);

	Integrate(dt);
	m_broadPhaseCollisionSystem.OnUpdate();
	m_narrowPhaseCollisionSystem.OnUpdate(m_broadPhaseCollisionSystem.GetCollisionPairs(), dt);

	FillTransformUpdateQueue();
}

void BodyPhysicsSystem::SyncWithTransform() const {
	auto &transformArray = ref_eM->GetCompArr<Scene::Components::Transform>();
	auto &rbArray		 = ref_eM->GetCompArr<Components::RigidBody>();

	for (auto const	   &updatedTransformEntityIDs = ref_transformSystem->GetUpdatedTransformEntityIDs();
		 const uint32_t entityID : updatedTransformEntityIDs) {
		if (rbArray.Has(entityID)) {
			Components::RigidBody			   &rb = rbArray.Get(entityID);
			const Scene::Components::Transform &tf = transformArray.Get(entityID);

			rb.position	   = tf.position;
			rb.orientation = tf.orientation;
			rb.UpdateDerivedData();
			rb.isDirty = false;
		}
	}
}

void BodyPhysicsSystem::UpdateKinematicBodies(const float dt) const {
	auto	   &kbData = ref_eM->GetCompArr<Components::KinematicBody>().Data();
	const float invDt  = 1.0f / dt;

	for (auto &kb : kbData) {
		// Linear velocity
		kb.velocity = (kb.targetPosition - kb.position) * invDt;
		kb.position = kb.targetPosition;

		// Angular velocity (shortest path quaternion difference)
		Math::RQuat deltaQ = kb.targetOrientation * Math::Conjugate(kb.orientation);

		if (deltaQ.w < 0.0f) {
			deltaQ.x = -deltaQ.x;
			deltaQ.y = -deltaQ.y;
			deltaQ.z = -deltaQ.z;
			deltaQ.w = -deltaQ.w;
		}

		if (const Math::real angle = 2.0f * std::acos(Math::Clamp(deltaQ.w, -1.0f, 1.0f)); angle > 0.001f) {
			Math::RVec3 axis(deltaQ.x, deltaQ.y, deltaQ.z);
			kb.angularVelocity = Math::Normalize(axis) * (angle * invDt);
		} else
			kb.angularVelocity = Math::RVec3(0.0f);

		kb.orientation = kb.targetOrientation;
	}
}

void BodyPhysicsSystem::Integrate(const float dt) const {
	auto							   &rbArray		= ref_eM->GetCompArr<Components::RigidBody>();
	std::vector<Components::RigidBody> &rigidBodies = rbArray.Data();
	const uint32_t						rbCount		= rbArray.GetCount();

	for (uint32_t i = 0; i < rbCount; ++i) {
		Components::RigidBody &rb = rigidBodies[i];

		// All RigidBodies should be processed by TransformSystem
		rb.isDirty = false;

		// We don't integrate if isn't awake or mass is zero.
		if ((rb.canSleep && !rb.isAwake) || rb.inverseMass <= 0.0f) continue;

		// Calculate linear acceleration from force inputs and gravity.
		rb.lastFrameAcceleration =
			rb.acceleration + (rb.gravityScale * GravityAcceleration) + (rb.forceAccum * rb.inverseMass);

		// Calculate angular acceleration from torque inputs.
		Math::RVec3 angularAcceleration = rb.inverseInertiaTensorWorld * rb.torqueAccum;

		// Adjust velocities.
		// Update linear velocity from both acceleration and impulse.
		rb.velocity += rb.lastFrameAcceleration * dt;

		// Update angular velocity from both acceleration and impulse.
		rb.angularVelocity += angularAcceleration * dt;

		// Impose drag
		rb.velocity *= 1.0 / (1.0f + (rb.linearDamping * dt));
		rb.angularVelocity *= 1.0 / (1.0f + (rb.angularDamping * dt));

		// Adjust positions.
		// Update linear position.
		rb.position += rb.velocity * dt;

		// Update angular position.
		IntegrateAngularPosition(rb, dt);

		// Update the inverse inertia tensor world with the new orientation and set true to isDirty flag
		rb.UpdateDerivedData();

		// Clear accumulators.
		rb.ClearAccumulators();

		if (rb.canSleep) {
			const Math::real linearSq	   = Math::Dot(rb.velocity, rb.velocity);
			const Math::real angularSq	   = Math::Dot(rb.angularVelocity, rb.angularVelocity);
			const Math::real currentMotion = linearSq + angularSq;

			const Math::real bias = Math::Pow(0.5f, dt);
			rb.motion			  = bias * rb.motion + (1.0 - bias) * currentMotion;

			if (rb.motion < Components::SLEEP_EPSILON)
				rb.Sleep();
			else if (rb.motion > 10 * Components::SLEEP_EPSILON)
				rb.motion = 10 * Components::SLEEP_EPSILON;
		}
	}
}

void BodyPhysicsSystem::IntegrateAngularPosition(Components::RigidBody &rb, const float dt) const {
	// 1. Convert angular velocity and dt into a Pure Quaternion (w=0)
	Math::RQuat q(0.0f, rb.angularVelocity.x * dt, rb.angularVelocity.y * dt, rb.angularVelocity.z * dt);

	// 2. Multiply by the current orientation (Find the derivative)
	q *= rb.orientation;

	// 3. Take half of it and add to the current orientation
	rb.orientation.w += q.w * 0.5f;
	rb.orientation.x += q.x * 0.5f;
	rb.orientation.y += q.y * 0.5f;
	rb.orientation.z += q.z * 0.5f;
}

void BodyPhysicsSystem::FillTransformUpdateQueue() {
	auto									 &rbArray	  = ref_eM->GetCompArr<Components::RigidBody>();
	const uint32_t							  rbCount	  = rbArray.GetCount();
	const std::vector<Components::RigidBody> &rigidBodies = rbArray.Data();
	const std::vector<ECS::EntityID>		 &rbEntityIDs = rbArray.Index();

	for (uint32_t i = 0; i < rbCount; ++i) {
		if (rigidBodies[i].isAwake || rigidBodies[i].isDirty)
			m_transformQueue.emplace_back(rbEntityIDs[i], rigidBodies[i].position, rigidBodies[i].orientation);
	}

	auto										 &kbArray	  = ref_eM->GetCompArr<Components::KinematicBody>();
	const uint32_t								  kbCount	  = kbArray.GetCount();
	const std::vector<Components::KinematicBody> &kinematics  = kbArray.Data();
	const std::vector<ECS::EntityID>			 &kbEntityIDs = kbArray.Index();

	// TODO: Add isDirty
	for (uint32_t i = 0; i < kbCount; ++i) {
		m_transformQueue.emplace_back(kbEntityIDs[i], kinematics[i].position, kinematics[i].orientation);
	}
}
}  // namespace PE::Physics::Body::Systems