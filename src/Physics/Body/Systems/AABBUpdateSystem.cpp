#include "Physics/Body/Systems/AABBUpdateSystem.h"

#include <algorithm>

#include "Physics/Body/Types.h"
#include "Physics/Body/Components/AABB.h"
#include "Physics/Body/Components/BoxCollider.h"
#include "Physics/Body/Components/CapsuleCollider.h"
#include "Physics/Body/Components/CylinderCollider.h"
#include "Physics/Body/Components/SphereCollider.h"
#include "Physics/Core/Systems/PhysicsSystem.h"
#include "Scene/Systems/TransformSystem.h"

namespace PE::Physics::Body::Systems {
AABBUpdateSystem::~AABBUpdateSystem() { AABBUpdateSystem::Shutdown(); }

ERROR_CODE AABBUpdateSystem::Initialize(const ECS::ESystemStage stage, ECS::ECSManager *ecsManager,
										Scene::Systems::TransformSystem *transformSystem,
										Core::Systems::PhysicsSystem	*physicsSystem) {
	PE_CHECK_STATE_INIT(m_state, "AABB Update system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID			= GetUniqueISystemTypeID<AABBUpdateSystem>();
	ref_eM				= ecsManager;
	ref_transformSystem = transformSystem;
	ref_physicsSystem	= physicsSystem;
	m_stage				= stage;

	ERROR_CODE result;
	PE_CHECK(result, ref_eM->RegisterSystem(this));
	return result;
}

ERROR_CODE AABBUpdateSystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	ERROR_CODE result;
	PE_CHECK(result, ref_eM->UnregisterSystem(this));
	m_stage	 = ECS::ESystemStage::Count;
	m_typeID = UINT32_MAX;

	m_state = SystemState::Uninitialized;
	return result;
}

void AABBUpdateSystem::OnUpdate(const float dt) {
	const auto &updatedEntitiesFromTransform   = ref_transformSystem->GetUpdatedTransformEntityIDs();
	const auto &updatedEntitiesFromBodyPhysics = ref_physicsSystem->GetBodyTransformQueue();

	auto &aabbArray		 = ref_eM->GetCompArr<Components::AABB>();
	auto &transformArray = ref_eM->GetCompArr<Scene::Components::Transform>();

	for (const TransformUpdateCommand &command : updatedEntitiesFromBodyPhysics) {
		const ECS::EntityID entityID = command.id;
		if (!aabbArray.Has(entityID)) continue;

		Components::AABB				   &aabb	  = aabbArray.Get(entityID);
		const Scene::Components::Transform &transform = transformArray.Get(entityID);

		if (auto &boxArr = ref_eM->GetCompArr<Components::BoxCollider>(); boxArr.Has(entityID)) {
			UpdateAABB(aabb, boxArr.Get(entityID), transform.worldMatrix);
		} else if (auto &sphereArr = ref_eM->GetCompArr<Components::SphereCollider>(); sphereArr.Has(entityID)) {
			UpdateAABB(aabb, sphereArr.Get(entityID), transform.worldMatrix);
		} else if (auto &capsuleArr = ref_eM->GetCompArr<Components::CapsuleCollider>(); capsuleArr.Has(entityID)) {
			UpdateAABB(aabb, capsuleArr.Get(entityID), transform.worldMatrix);
		} else if (auto &cylinderArr = ref_eM->GetCompArr<Components::CylinderCollider>(); cylinderArr.Has(entityID)) {
			UpdateAABB(aabb, cylinderArr.Get(entityID), transform.worldMatrix);
		}
	}

	for (const uint32_t entityID : updatedEntitiesFromTransform) {
		if (!aabbArray.Has(entityID)) continue;

		Components::AABB				   &aabb	  = aabbArray.Get(entityID);
		const Scene::Components::Transform &transform = transformArray.Get(entityID);

		if (auto &boxArr = ref_eM->GetCompArr<Components::BoxCollider>(); boxArr.Has(entityID)) {
			UpdateAABB(aabb, boxArr.Get(entityID), transform.worldMatrix);
		} else if (auto &sphereArr = ref_eM->GetCompArr<Components::SphereCollider>(); sphereArr.Has(entityID)) {
			UpdateAABB(aabb, sphereArr.Get(entityID), transform.worldMatrix);
		} else if (auto &capsuleArr = ref_eM->GetCompArr<Components::CapsuleCollider>(); capsuleArr.Has(entityID)) {
			UpdateAABB(aabb, capsuleArr.Get(entityID), transform.worldMatrix);
		} else if (auto &cylinderArr = ref_eM->GetCompArr<Components::CylinderCollider>(); cylinderArr.Has(entityID)) {
			UpdateAABB(aabb, cylinderArr.Get(entityID), transform.worldMatrix);
		}
	}
}

void AABBUpdateSystem::UpdateAABB(Components::AABB &outAABB, Components::BoxCollider &boxCol,
												const Math::Mat44 &m) {
	const float scaleX = Math::Sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);
	const float scaleY = Math::Sqrt(m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]);
	const float scaleZ = Math::Sqrt(m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]);

	boxCol.worldHalfExtents.x = boxCol.localHalfExtents.x * scaleX;
	boxCol.worldHalfExtents.y = boxCol.localHalfExtents.y * scaleY;
	boxCol.worldHalfExtents.z = boxCol.localHalfExtents.z * scaleZ;

	const float localOffsetX = boxCol.localOffset.x;
	const float localOffsetY = boxCol.localOffset.y;
	const float localOffsetZ = boxCol.localOffset.z;

	const float worldOffsetX = (m[0][0] * localOffsetX) + (m[1][0] * localOffsetY) + (m[2][0] * localOffsetZ);
	const float worldOffsetY = (m[0][1] * localOffsetX) + (m[1][1] * localOffsetY) + (m[2][1] * localOffsetZ);
	const float worldOffsetZ = (m[0][2] * localOffsetX) + (m[1][2] * localOffsetY) + (m[2][2] * localOffsetZ);

	const float centerX = m[3][0] + worldOffsetX;
	const float centerY = m[3][1] + worldOffsetY;
	const float centerZ = m[3][2] + worldOffsetZ;

	const float localHalfExtentX = boxCol.localHalfExtents.x;
	const float localHalfExtentY = boxCol.localHalfExtents.y;
	const float localHalfExtentZ = boxCol.localHalfExtents.z;

	const float extentX = (Math::Abs(m[0][0]) * localHalfExtentX) + (Math::Abs(m[1][0]) * localHalfExtentY) +
						  (Math::Abs(m[2][0]) * localHalfExtentZ);
	const float extentY = (Math::Abs(m[0][1]) * localHalfExtentX) + (Math::Abs(m[1][1]) * localHalfExtentY) +
						  (Math::Abs(m[2][1]) * localHalfExtentZ);
	const float extentZ = (Math::Abs(m[0][2]) * localHalfExtentX) + (Math::Abs(m[1][2]) * localHalfExtentY) +
						  (Math::Abs(m[2][2]) * localHalfExtentZ);

	outAABB.min = Math::RVec3(centerX - extentX, centerY - extentY, centerZ - extentZ);
	outAABB.max = Math::RVec3(centerX + extentX, centerY + extentY, centerZ + extentZ);
}

void AABBUpdateSystem::UpdateAABB(Components::AABB &outAABB, Components::CapsuleCollider &capCol,
													const Math::Mat44 &m) {
	const float scaleX = Math::Sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);
	const float scaleY = Math::Sqrt(m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]);
	const float scaleZ = Math::Sqrt(m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]);

	capCol.worldHalfHeight				 = std::max(static_cast<Math::real>(0.0), (capCol.localHalfHeight * scaleY) - capCol.worldRadius);
	// Radius must remain circular, so we take the maximum of X and Z scales
	capCol.worldRadius					 = capCol.localRadius * std::max(scaleX, scaleZ);

	const float localOffsetX = capCol.localOffset.x;
	const float localOffsetY = capCol.localOffset.y;
	const float localOffsetZ = capCol.localOffset.z;

	const float worldOffsetX = (m[0][0] * localOffsetX) + (m[1][0] * localOffsetY) + (m[2][0] * localOffsetZ);
	const float worldOffsetY = (m[0][1] * localOffsetX) + (m[1][1] * localOffsetY) + (m[2][1] * localOffsetZ);
	const float worldOffsetZ = (m[0][2] * localOffsetX) + (m[1][2] * localOffsetY) + (m[2][2] * localOffsetZ);

	const float centerX = m[3][0] + worldOffsetX;
	const float centerY = m[3][1] + worldOffsetY;
	const float centerZ = m[3][2] + worldOffsetZ;

	const float invScaleY = scaleY > Math::REpsilon ? static_cast<float>(1.0) / scaleY : 0.0f;

	const float upAxisX = m[1][0] * invScaleY * capCol.worldHalfHeight;
	const float upAxisY = m[1][1] * invScaleY * capCol.worldHalfHeight;
	const float upAxisZ = m[1][2] * invScaleY * capCol.worldHalfHeight;

	const float topX = centerX + upAxisX;
	const float topY = centerY + upAxisY;
	const float topZ = centerZ + upAxisZ;

	const float botX = centerX - upAxisX;
	const float botY = centerY - upAxisY;
	const float botZ = centerZ - upAxisZ;

	outAABB.min.x = std::min(topX, botX) - capCol.worldRadius;
	outAABB.min.y = std::min(topY, botY) - capCol.worldRadius;
	outAABB.min.z = std::min(topZ, botZ) - capCol.worldRadius;

	outAABB.max.x = std::max(topX, botX) + capCol.worldRadius;
	outAABB.max.y = std::max(topY, botY) + capCol.worldRadius;
	outAABB.max.z = std::max(topZ, botZ) + capCol.worldRadius;
}

void AABBUpdateSystem::UpdateAABB(Components::AABB &outAABB, Components::CylinderCollider &cylinderCol,
													 const Math::Mat44 &m) {
	const float scaleX = Math::Sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);
	const float scaleY = Math::Sqrt(m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]);
	const float scaleZ = Math::Sqrt(m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]);

	// Height is determined by the local Y scale
	cylinderCol.worldHalfHeight = cylinderCol.localHalfHeight * scaleY;
	// Radius must remain circular, so we take the maximum of X and Z scales
	cylinderCol.worldRadius = cylinderCol.localRadius * std::max(scaleX, scaleZ);

	const float localOffsetX = cylinderCol.localOffset.x;
	const float localOffsetY = cylinderCol.localOffset.y;
	const float localOffsetZ = cylinderCol.localOffset.z;

	const float worldOffsetX = (m[0][0] * localOffsetX) + (m[1][0] * localOffsetY) + (m[2][0] * localOffsetZ);
	const float worldOffsetY = (m[0][1] * localOffsetX) + (m[1][1] * localOffsetY) + (m[2][1] * localOffsetZ);
	const float worldOffsetZ = (m[0][2] * localOffsetX) + (m[1][2] * localOffsetY) + (m[2][2] * localOffsetZ);

	const float centerX = m[3][0] + worldOffsetX;
	const float centerY = m[3][1] + worldOffsetY;
	const float centerZ = m[3][2] + worldOffsetZ;

	const float invScaleY = scaleY > Math::REpsilon ? static_cast<float>(1.0) / scaleY : 0.0f;

	const float upAxisX = m[1][0] * invScaleY;
	const float upAxisY = m[1][1] * invScaleY;
	const float upAxisZ = m[1][2] * invScaleY;

	const float axialExtentX = Math::Abs(upAxisX) * cylinderCol.worldHalfHeight;
	const float axialExtentY = Math::Abs(upAxisY) * cylinderCol.worldHalfHeight;
	const float axialExtentZ = Math::Abs(upAxisZ) * cylinderCol.worldHalfHeight;

	const float radialExtentX = cylinderCol.worldRadius * Math::Sqrt(std::max(0.0f, 1.0f - upAxisX * upAxisX));
	const float radialExtentY = cylinderCol.worldRadius * Math::Sqrt(std::max(0.0f, 1.0f - upAxisY * upAxisY));
	const float radialExtentZ = cylinderCol.worldRadius * Math::Sqrt(std::max(0.0f, 1.0f - upAxisZ * upAxisZ));

	const float extentX = axialExtentX + radialExtentX;
	const float extentY = axialExtentY + radialExtentY;
	const float extentZ = axialExtentZ + radialExtentZ;

	outAABB.min = Math::RVec3(centerX - extentX, centerY - extentY, centerZ - extentZ);
	outAABB.max = Math::RVec3(centerX + extentX, centerY + extentY, centerZ + extentZ);
}

void AABBUpdateSystem::UpdateAABB(Components::AABB &outAABB, Components::SphereCollider &sphereCol,
												   const Math::Mat44 &m) {
	const float scaleX = Math::Sqrt(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);
	const float scaleY = Math::Sqrt(m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]);
	const float scaleZ = Math::Sqrt(m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]);

	const float localOffsetX = sphereCol.localOffset.x;
	const float localOffsetY = sphereCol.localOffset.y;
	const float localOffsetZ = sphereCol.localOffset.z;

	const float worldOffsetX = (m[0][0] * localOffsetX) + (m[1][0] * localOffsetY) + (m[2][0] * localOffsetZ);
	const float worldOffsetY = (m[0][1] * localOffsetX) + (m[1][1] * localOffsetY) + (m[2][1] * localOffsetZ);
	const float worldOffsetZ = (m[0][2] * localOffsetX) + (m[1][2] * localOffsetY) + (m[2][2] * localOffsetZ);

	const float centerX = m[3][0] + worldOffsetX;
	const float centerY = m[3][1] + worldOffsetY;
	const float centerZ = m[3][2] + worldOffsetZ;

	// We use the maximum scale component to keep the shape spherical and conservative
	const float maxScale = std::max({scaleX, scaleY, scaleZ});
	sphereCol.worldRadius	 = sphereCol.localRadius * maxScale;

	const float extentMax = sphereCol.worldRadius;

	outAABB.min = Math::RVec3(centerX - extentMax, centerY - extentMax, centerZ - extentMax);
	outAABB.max = Math::RVec3(centerX + extentMax, centerY + extentMax, centerZ + extentMax);
}
}  // namespace PE::Physics::Body::Systems