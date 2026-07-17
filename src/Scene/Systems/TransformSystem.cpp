#include "Scene/Systems/TransformSystem.h"

#include "Graphics/Systems/CameraSystem.h"
#include "Physics/Core/Systems/PhysicsSystem.h"
#include "Scene/Components/Transform.h"

namespace PE::Scene::Systems {
ERROR_CODE TransformSystem::Initialize(const ECS::ESystemStage stage, ECS::ECSManager *ecsManager,
									   const Physics::Core::Systems::PhysicsSystem &physicsSystem,
									   const Core::EngineConfig					   &config) {
	PE_CHECK_STATE_INIT(m_state, "Transform system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID		  = GetUniqueISystemTypeID<TransformSystem>();
	ref_eM			  = ecsManager;
	ref_config		  = &config;
	ref_physicsSystem = &physicsSystem;
	m_stage			  = stage;

	m_updatedTransformEntityIDs.reserve(config.maxEntityCount);

	ERROR_CODE result;
	PE_CHECK(result, ref_eM->RegisterSystem(this));
	m_state = SystemState::Running;

	return result;
}

ERROR_CODE TransformSystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	ERROR_CODE result;
	PE_CHECK(result, ref_eM->UnregisterSystem(this));
	m_stage	 = ECS::ESystemStage::Count;
	m_typeID = UINT32_MAX;
	m_state	 = SystemState::Uninitialized;

	return ERROR_CODE::OK;
}

void TransformSystem::OnUpdate(float dt) {
	// TODO: This might be create bug because of it's come before RebuildTransformArray
	for (const auto &updateQueue = ref_physicsSystem->GetParticleTransformQueue();
		 const auto &[entity, newPosition] : updateQueue) {
		if (ref_eM->HasComponent<Components::Transform>(entity.id))
			SyncPosition(entity.id, newPosition);
	}

	for (const auto &updateQueue = ref_physicsSystem->GetBodyTransformQueue();
		 const auto &[id, newPosition, newOrientation] : updateQueue) {
		if (ref_eM->HasComponent<Components::Transform>(id))
			SyncPositionAndOrientation(id, newPosition, newOrientation);
	}

	auto &compArr = ref_eM->GetCompArr<Components::Transform>();
	using Components::Transform;
	if (compArr.GetCount() != m_lastComponentCount) m_isHierarchyDirty = true;

	if (m_isHierarchyDirty) {
		RebuildTransformArray();
		m_isHierarchyDirty = false;

		m_lastComponentCount = compArr.GetCount();
	}

	const uint32_t transformCount = compArr.GetCount();

	std::vector<Transform> &transforms = compArr.Data();

	for (uint32_t i = 0; i < transformCount; ++i) {
		if (transforms[i].state == Transform::TransformState::Updated)
			transforms[i].state = Transform::TransformState::Clean;
	}

	m_updatedTransformEntityIDs.clear();
	for (uint32_t i = 0; i < transformCount; ++i) {
		ECS::EntityID entityId = compArr.Index()[i];
		if (Transform &transform = transforms[i]; transform.parentPackedIndex != UINT32_MAX) {
			Transform const &parent = transforms[transform.parentPackedIndex];

			if (parent.state == Transform::TransformState::Updated || parent.state == Transform::TransformState::Sync)
				transform.state = Transform::TransformState::Dirty;
			if (transform.state == Transform::TransformState::Dirty) {
				ProcessDirtyTransform(entityId, transform, parent);
			} else if (transform.state == Transform::TransformState::Sync) {
				UpdateWorldMatrix(transform, parent);
				transform.state = Transform::TransformState::Clean;
			}
		} else {
			if (transform.state == Transform::TransformState::Dirty) {
				ProcessDirtyTransform(entityId, transform);
			} else if (transform.state == Transform::TransformState::Sync) {
				UpdateWorldMatrix(transform);
				transform.state = Transform::TransformState::Clean;
			}
		}
	}
}

void TransformSystem::ResetSyncData() {
	m_updatedTransformEntityIDs.clear();
}

void TransformSystem::SetPosition(const uint32_t entityID, const float x, const float y, const float z) const {
	auto &transform		 = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.position.x = x;
	transform.position.y = y;
	transform.position.z = z;
	transform.state		 = Components::Transform::TransformState::Dirty;
}

void TransformSystem::SetPosition(const uint32_t entityID, const Math::Vec3 pos) const {
	auto &transform	   = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.position = pos;
	transform.state	   = Components::Transform::TransformState::Dirty;
}

Math::Vec3 TransformSystem::GetPosition(const uint32_t entityID) const {
	return ref_eM->GetCompArr<Components::Transform>().Get(entityID).position;
}

void TransformSystem::SyncPosition(const uint32_t entityID, const float x, const float y, const float z) const {
	auto &transform		 = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.position.x = x;
	transform.position.y = y;
	transform.position.z = z;

	transform.state = Components::Transform::TransformState::Sync;
}

void TransformSystem::SyncPosition(const uint32_t entityID, const Math::Vec3 pos) const {
	auto &transform	   = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.position = pos;
	transform.state	   = Components::Transform::TransformState::Sync;
}

void TransformSystem::SyncPositionAndOrientation(const uint32_t entityID, const Math::Vec3 pos,
												 const Math::Quat orientation) const {
	auto &transform		  = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.position	  = pos;
	transform.orientation = orientation;
	transform.state		  = Components::Transform::TransformState::Sync;
}

void TransformSystem::SetOrientation(const uint32_t entityID, const float pitch, const float yaw,
									 const float roll) const {
	auto &transform		  = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.orientation = Math::Quat(Math::Vec3(pitch, yaw, roll));
	transform.state		  = Components::Transform::TransformState::Dirty;
}

void TransformSystem::SetOrientation(const uint32_t entityID, const Math::Quat rot) const {
	auto &transform		  = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.orientation = rot;
	transform.state		  = Components::Transform::TransformState::Dirty;
}

void TransformSystem::SetOrientation(const uint32_t entityID, const Math::Vec3 radOrientation) const {
	auto &transform		  = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.orientation = Math::EulerToQuat(radOrientation);
	transform.state		  = Components::Transform::TransformState::Dirty;
}

Math::Quat TransformSystem::GetOrientation(const uint32_t entityID) const {
	return ref_eM->GetCompArr<Components::Transform>().Get(entityID).orientation;
}

void TransformSystem::SetScale(const uint32_t entityID, const float x, const float y, const float z) const {
	auto &transform	  = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.scale.x = x;
	transform.scale.y = y;
	transform.scale.z = z;
	transform.state	  = Components::Transform::TransformState::Dirty;
}

void TransformSystem::SetScale(const uint32_t entityID, const Math::Vec3 scale) const {
	auto &transform = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.scale = scale;
	transform.state = Components::Transform::TransformState::Dirty;
}

Math::Vec3 TransformSystem::GetScale(const uint32_t entityID) const {
	return ref_eM->GetCompArr<Components::Transform>().Get(entityID).scale;
}

void TransformSystem::SetPositionAndOrientation(const uint32_t entityID, const Math::Vec3 position,
												const Math::Quat orientation) const {
	auto &transform		  = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.position	  = position;
	transform.orientation = orientation;
	transform.state		  = Components::Transform::TransformState::Dirty;
}

void TransformSystem::ProcessDirtyTransform(const ECS::EntityID entityId, Components::Transform &transform) {
	UpdateWorldMatrix(transform);
	transform.state = Components::Transform::TransformState::Updated;
	m_updatedTransformEntityIDs.push_back(entityId);
}

void TransformSystem::ProcessDirtyTransform(const ECS::EntityID entityId, Components::Transform &transform,
											Components::Transform const &parent) {
	UpdateWorldMatrix(transform, parent);
	transform.state = Components::Transform::TransformState::Updated;
	m_updatedTransformEntityIDs.push_back(entityId);
}

void TransformSystem::UpdateWorldMatrix(Components::Transform &t) {
	const float xx = t.orientation.x * t.orientation.x;
	const float yy = t.orientation.y * t.orientation.y;
	const float zz = t.orientation.z * t.orientation.z;
	const float xy = t.orientation.x * t.orientation.y;
	const float xz = t.orientation.x * t.orientation.z;
	const float yz = t.orientation.y * t.orientation.z;
	const float wx = t.orientation.w * t.orientation.x;
	const float wy = t.orientation.w * t.orientation.y;
	const float wz = t.orientation.w * t.orientation.z;

	t.localMatrix[0][0] = (1.0f - 2.0f * (yy + zz)) * t.scale.x;
	t.localMatrix[0][1] = (2.0f * (xy + wz)) * t.scale.x;
	t.localMatrix[0][2] = (2.0f * (xz - wy)) * t.scale.x;
	t.localMatrix[0][3] = 0.0f;

	t.localMatrix[1][0] = (2.0f * (xy - wz)) * t.scale.y;
	t.localMatrix[1][1] = (1.0f - 2.0f * (xx + zz)) * t.scale.y;
	t.localMatrix[1][2] = (2.0f * (yz + wx)) * t.scale.y;
	t.localMatrix[1][3] = 0.0f;

	t.localMatrix[2][0] = (2.0f * (xz + wy)) * t.scale.z;
	t.localMatrix[2][1] = (2.0f * (yz - wx)) * t.scale.z;
	t.localMatrix[2][2] = (1.0f - 2.0f * (xx + yy)) * t.scale.z;
	t.localMatrix[2][3] = 0.0f;

	t.localMatrix[3][0] = t.position.x;
	t.localMatrix[3][1] = t.position.y;
	t.localMatrix[3][2] = t.position.z;
	t.localMatrix[3][3] = 1.0f;
	t.worldMatrix		= t.localMatrix;
}

void TransformSystem::UpdateWorldMatrix(Components::Transform &t, const Components::Transform &parentTransform) {
	const float xx = t.orientation.x * t.orientation.x;
	const float yy = t.orientation.y * t.orientation.y;
	const float zz = t.orientation.z * t.orientation.z;
	const float xy = t.orientation.x * t.orientation.y;
	const float xz = t.orientation.x * t.orientation.z;
	const float yz = t.orientation.y * t.orientation.z;
	const float wx = t.orientation.w * t.orientation.x;
	const float wy = t.orientation.w * t.orientation.y;
	const float wz = t.orientation.w * t.orientation.z;

	t.localMatrix[0][0] = (1.0f - 2.0f * (yy + zz)) * t.scale.x;
	t.localMatrix[0][1] = (2.0f * (xy + wz)) * t.scale.x;
	t.localMatrix[0][2] = (2.0f * (xz - wy)) * t.scale.x;
	t.localMatrix[0][3] = 0.0f;

	t.localMatrix[1][0] = (2.0f * (xy - wz)) * t.scale.y;
	t.localMatrix[1][1] = (1.0f - 2.0f * (xx + zz)) * t.scale.y;
	t.localMatrix[1][2] = (2.0f * (yz + wx)) * t.scale.y;
	t.localMatrix[1][3] = 0.0f;

	t.localMatrix[2][0] = (2.0f * (xz + wy)) * t.scale.z;
	t.localMatrix[2][1] = (2.0f * (yz - wx)) * t.scale.z;
	t.localMatrix[2][2] = (1.0f - 2.0f * (xx + yy)) * t.scale.z;
	t.localMatrix[2][3] = 0.0f;

	t.localMatrix[3][0] = t.position.x;
	t.localMatrix[3][1] = t.position.y;
	t.localMatrix[3][2] = t.position.z;
	t.localMatrix[3][3] = 1.0f;
	t.worldMatrix		= parentTransform.worldMatrix * t.localMatrix;
}

void TransformSystem::AttachEntity(const uint32_t childEntityID, const uint32_t parentEntityID) {
	auto &array = ref_eM->GetCompArr<Components::Transform>();
	if (!array.Has(childEntityID) || !array.Has(parentEntityID)) return;

	array.Get(childEntityID).parentEntityID = parentEntityID;

	m_isHierarchyDirty = true;
}

void TransformSystem::DetachEntity(const uint32_t entityID) {
	auto &array = ref_eM->GetCompArr<Components::Transform>();
	if (!array.Has(entityID)) return;

	array.Get(entityID).parentEntityID = UINT32_MAX;
	m_isHierarchyDirty				   = true;
}

void TransformSystem::RebuildTransformArray() {
	auto		  &array = ref_eM->GetCompArr<Components::Transform>();
	const uint32_t count = array.GetCount();
	if (count == 0) return;

	static std::vector<std::vector<uint32_t>> adj;

	if (adj.size() < ref_config->maxEntityCount) adj.resize(ref_config->maxEntityCount);

	const auto &indices = array.Index();
	for (const uint32_t entityID : indices) {
		if (entityID >= adj.size()) PE_LOG_FATAL("This vector can't be bigger than max entity count!");

		adj[entityID].clear();
	}

	std::vector<uint32_t> roots;
	roots.reserve(count / 2);

	for (uint32_t i = 0; i < count; ++i) {
		uint32_t entityID = indices[i];
		if (const auto &transform = array.Data()[i]; transform.parentEntityID == UINT32_MAX) {
			roots.push_back(entityID);
		} else {
			if (transform.parentEntityID < adj.size())
				adj[transform.parentEntityID].push_back(entityID);
			else
				PE_LOG_FATAL("This entity ID can't be bigger than max entity count!");
		}
	}

	static std::vector<Components::Transform> sortedData;
	static std::vector<uint32_t>			  sortedEntities;

	sortedData.clear();
	sortedData.reserve(count);
	sortedEntities.clear();
	sortedEntities.reserve(count);

	for (uint32_t rootID : roots) {
		DFSRebuild(rootID, UINT32_MAX, adj, sortedData, sortedEntities);
	}

	array.Shutdown();

	array.Initialize(ref_config->maxEntityCount);

	for (size_t i = 0; i < sortedData.size(); ++i) {
		array.Add(sortedEntities[i], &sortedData[i]);
	}

	m_lastComponentCount = array.GetCount();
}

void TransformSystem::DFSRebuild(uint32_t entityID, uint32_t currentParentPackedIndex,
								 const std::vector<std::vector<uint32_t>> &adj,
								 std::vector<Components::Transform>		  &sortedData,
								 std::vector<uint32_t>					  &sortedEntities) {
	auto				 &array		= ref_eM->GetCompArr<Components::Transform>();
	Components::Transform transform = array.Get(entityID);

	const uint32_t myNewPackedIndex = static_cast<uint32_t>(sortedData.size());

	transform.parentPackedIndex = currentParentPackedIndex;

	sortedData.push_back(transform);
	sortedEntities.push_back(entityID);

	if (entityID < adj.size()) {
		for (uint32_t childID : adj[entityID]) {
			DFSRebuild(childID, myNewPackedIndex, adj, sortedData, sortedEntities);
		}
	}
}
}  // namespace PE::Scene::Systems