#include "Scene/Systems/SpawnSystem.h"

#include <format>
#include <random>

#include "Assets/AssetManager.h"
#include "Graphics/Components/MeshRenderer.h"
#include "Graphics/RenderTypes.h"
#include "Math/Random.h"
#include "Physics/Body/Components/AABB.h"
#include "Physics/Body/Components/BoxCollider.h"
#include "Physics/Body/Components/CapsuleCollider.h"
#include "Physics/Body/Components/CylinderCollider.h"
#include "Physics/Body/Components/PhysicsMaterial.h"
#include "Physics/Body/Components/RigidBody.h"
#include "Physics/Body/Components/SphereCollider.h"
#include "Scene/Components/Tag.h"
#include "Scene/Components/Transform.h"

namespace PE::Scene::Systems {
static constexpr std::string OwnerMatNames[4] = {"Mat_Owner_One", "Mat_Owner_Two", "Mat_Owner_Three", "Mat_Owner_Four"};

void SpawnSystem::Initialize(ECS::ECSManager *ecsManager) { ref_eM = ecsManager; }

void SpawnSystem::Shutdown() {}

void SpawnSystem::OnUpdate(const float dt) {
	m_timePassed += dt;
	ProcessSpawner(dt);

	for (const ECS::EntityID id : removeIndex) {
		ref_eM->RemoveComponent<Components::Spawner>(id);
	}
	removeIndex.clear();
}

void SpawnSystem::Reset() { m_timePassed = 0; }

void SpawnSystem::ProcessSpawner(const float dt) {
	auto &spawnerCompArr = ref_eM->GetCompArr<Components::Spawner>();
	for (int i = spawnerCompArr.GetCount() - 1; i >= 0; i--) {
		auto &spawner = spawnerCompArr.Data()[i];

		if (spawner.startTime > m_timePassed) {
			continue;
		}

		if (auto &frequency = spawner.frequency; frequency.isSingleBurst) {
			SpawnSingleBurst(spawner);
			removeIndex.push_back(spawnerCompArr.Index()[i]);
		} else {
			if (auto &repeating = frequency.repeating; repeating.maxCount < 1) {
				removeIndex.push_back(spawnerCompArr.Index()[i]);
			} else {
				repeating.timePassedLastSpawn += dt;
				if (repeating.timePassedLastSpawn >= repeating.interval) {
					CreateSimulationEntity(spawner);
					repeating.maxCount--;
					repeating.timePassedLastSpawn -= repeating.interval;
				}
			}
		}
	}
}

void SpawnSystem::SpawnSingleBurst(const Components::Spawner &spawner) {
	uint32_t count = spawner.frequency.burst.count;
	while (count > 0) {
		CreateSimulationEntity(spawner);
		count--;
	}
}

void SpawnSystem::CreateSimulationEntity(const Components::Spawner &spawner) {
	const ECS::EntityID entityID = ref_eM->CreateEntity();
	CreateTagComponent(spawner.name, entityID);

	// Transform
	Components::Transform tf = CreateTransformComponent(spawner.location);

	// RigidBody
	Physics::Body::Components::RigidBody rb =
		CreateCommonRigidBody(tf, spawner.velocities.linear, spawner.velocities.angular, spawner.isGravityOn);
	const Math::real mass = spawner.density;
	// PhysicsMaterial
	ref_eM->AddComponent(entityID, Physics::Body::Components::PhysicsMaterial{spawner.physicsMatID});

	Graphics::MeshID meshID;
	const auto		&sizeRange = spawner.sizeRange;
	if (spawner.shape == Physics::Body::Components::ColliderShape::Sphere) {
		meshID			   = Assets::AssetManager::DefaultSphereID;
		const float radius = Math::Random::Get<float>(sizeRange.sphere.radius.min, sizeRange.sphere.radius.max);
		rb.SetMassAndInertiaTensorForSphereCollider(mass, radius);
		tf.scale *= radius;
	} else if (spawner.shape == Physics::Body::Components::ColliderShape::Box) {
		meshID					 = Assets::AssetManager::DefaultBoxID;
		const Math::Vec3 extents = Math::Random::GetPointInBox(sizeRange.cuboid.size.min, sizeRange.cuboid.size.max);
		rb.SetMassAndInertiaTensorForBoxCollider(mass, extents);
		tf.scale *= extents;
	} else {
		meshID = spawner.shape == Physics::Body::Components::ColliderShape::Capsule
					 ? Assets::AssetManager::DefaultCapsuleID
					 : Assets::AssetManager::DefaultCylinderID;
		const float radius =
			Math::Random::Get<float>(sizeRange.capsuleAndCylinder.radius.min, sizeRange.capsuleAndCylinder.radius.max);
		const float height =
			Math::Random::Get<float>(sizeRange.capsuleAndCylinder.height.min, sizeRange.capsuleAndCylinder.height.max);
		rb.SetMassAndInertiaTensorForCapsuleCollider(mass, radius, height);
		tf.scale *= Math::Vec3{radius * 2, height, radius * 2};
	}
	ref_eM->AddComponent(entityID, rb);

	// AABB and Collider
	CreateColliderAndAABBComponent(entityID, spawner.shape);

	CreateMeshRendererComponent(entityID, meshID);

	ref_eM->AddComponent(entityID, tf);
}

void SpawnSystem::CreateTagComponent(const std::string &name, ECS::EntityID entityID) const {
	// Tag
	const std::string entityName = std::format("{}_{}", name, entityID);
	ref_eM->AddComponent(entityID, Components::Tag{.name = entityName});
}

Components::Transform SpawnSystem::CreateTransformComponent(const Components::ULocation &location) {
	Components::Transform tf{};
	if (location.type == Components::ULocation::Type::FixedLocation) {
		tf.position	   = location.fixed.position;
		tf.orientation = Math::EulerToQuat(location.fixed.orientationEuler);
		tf.scale	   = location.fixed.scale;
	} else if (location.type == Components::ULocation::Type::RandomBox) {
		Math::Vec3 randPos = Math::Random::GetPointInBox(location.box.min, location.box.max);
		tf.position        = randPos;
	} else {
		Math::Vec3 randPos = Math::Random::GetPointInSphere(location.sphere.center, location.sphere.radius);
		tf.position        = randPos;
	}
	tf.state = Components::Transform::TransformState::Dirty;
	return tf;
}

Physics::Body::Components::RigidBody SpawnSystem::CreateCommonRigidBody(const Components::Transform &tf,
																		const Math::Vec3Range		&velRange,
																		const Math::Vec3Range		&angVelRange,
																		const bool					 isGravityOn) {
	Physics::Body::Components::RigidBody rb{};
	rb.position        = tf.position;
	rb.orientation     = tf.orientation;
	rb.velocity        = Math::Random::GetPointInBox(velRange.min, velRange.max);
	rb.angularVelocity = Math::Random::GetPointInBox(angVelRange.min, angVelRange.max);
	rb.gravityScale    = isGravityOn ? 1.0 : 0.0;
	rb.canSleep        = true;
	rb.UpdateDerivedData();
	return rb;
}

void SpawnSystem::CreateColliderAndAABBComponent(const ECS::EntityID							entityID,
												 const Physics::Body::Components::ColliderShape shape) const {
	Physics::Body::Components::AABB aabb{};
	aabb.SetShapeType(shape);
	ref_eM->AddComponent(entityID, aabb);
	switch (shape) {
		case Physics::Body::Components::ColliderShape::Box:
			ref_eM->AddComponent(entityID, Physics::Body::Components::BoxCollider{});
			break;
		case Physics::Body::Components::ColliderShape::Sphere:
			ref_eM->AddComponent(entityID, Physics::Body::Components::SphereCollider{});
			break;
		case Physics::Body::Components::ColliderShape::Capsule:
			ref_eM->AddComponent(entityID, Physics::Body::Components::CapsuleCollider{});
			break;
		case Physics::Body::Components::ColliderShape::Cylinder:
			ref_eM->AddComponent(entityID, Physics::Body::Components::CylinderCollider{});
			break;
		case Physics::Body::Components::ColliderShape::Count: break;
	}
}

void SpawnSystem::CreateMeshRendererComponent(const ECS::EntityID entityID,
											  const Graphics::MeshID meshID) const {
	Graphics::Components::MeshRenderer mr;
	mr.subMeshes.emplace_back();
	mr.subMeshes[0] = {.meshID = meshID, .materialID = Assets::AssetManager::GetMaterialHandle(Assets::AssetManager::DefaultMaterialName.data())};
	ref_eM->AddComponent(entityID, mr);
}
}  // namespace PE::Scene::Systems