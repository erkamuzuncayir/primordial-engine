#include "Scene/Systems/TransformSystem.h"

#include "Graphics/Systems/CameraSystem.h"
#include "Input/InputCode.h"
#include "Input/InputSystem.h"
#include "Input/InputTypes.h"
#include "Scene/Components/Transform.h"

namespace PE::Scene::Systems {
ERROR_CODE TransformSystem::Initialize(const ECS::ESystemStage stage, ECS::EntityManager *entityManager,
									   Input::InputSystem *inputSystem, Graphics::Systems::CameraSystem *cameraSystem,
									   const Core::EngineConfig &config) {
	PE_CHECK_STATE_INIT(m_state, "Transform system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID   = GetUniqueISystemTypeID<TransformSystem>();
	ref_eM	   = entityManager;
	ref_config = &config;
	m_stage	   = stage;

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

	return result;
}

void TransformSystem::OnUpdate(float dt) {
	using Components::Transform;
	auto &compArr = ref_eM->GetCompArr<Transform>();
	if (compArr.GetCount() != m_lastComponentCount) m_isHierarchyDirty = true;

	if (m_isHierarchyDirty) {
		RebuildTransformArray();
		m_isHierarchyDirty = false;

		m_lastComponentCount = compArr.GetCount();
	}

	const uint32_t transformCount = compArr.GetCount();

	std::vector<Transform> &transforms = compArr.Data();

	for (uint32_t i = 0; i < transformCount; ++i) {
		if (transforms[i].state == Transform::TransformState::Updated) {
			transforms[i].state = Transform::TransformState::Clean;
		}
	}

	for (uint32_t i = 0; i < transformCount; ++i) {
		Transform &transform = transforms[i];

		if (transform.parentPackedIndex != UINT32_MAX) {
			Transform const &parent = transforms[transform.parentPackedIndex];

			if (parent.state == Transform::TransformState::Updated) {
				transform.state = Transform::TransformState::Dirty;
			}

			if (transform.state == Transform::TransformState::Dirty) {
				UpdateWorldMatrix(transform, parent);
				transform.state = Transform::TransformState::Updated;
			}
		} else {
			if (transform.state == Transform::TransformState::Dirty) {
				UpdateWorldMatrix(transform);
				transform.state = Transform::TransformState::Updated;
			}
		}
	}
}

void TransformSystem::SetPosition(const uint32_t entityID, const float x, const float y, const float z) const {
	auto &transform	   = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.position = Math::Vec3(x, y, z);
	transform.state	   = Components::Transform::TransformState::Dirty;
}

void TransformSystem::SetPosition(const uint32_t entityID, const Math::Vec3 pos) const {
	auto &transform	   = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.position = pos;
	transform.state	   = Components::Transform::TransformState::Dirty;
}

Math::Vec3 TransformSystem::GetPosition(const uint32_t entityID) const {
	return ref_eM->GetCompArr<Components::Transform>().Get(entityID).position;
}

void TransformSystem::SetRotation(const uint32_t entityID, const float pitch, const float yaw, const float roll) const {
	auto &transform	   = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.rotation = Math::Vec3(pitch, yaw, roll);
	transform.state	   = Components::Transform::TransformState::Dirty;
}

void TransformSystem::SetRotation(const uint32_t entityID, const Math::Vec3 rot) const {
	auto &transform	   = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.rotation = rot;
	transform.state	   = Components::Transform::TransformState::Dirty;
}

Math::Vec3 TransformSystem::GetRotation(const uint32_t entityID) const {
	return ref_eM->GetCompArr<Components::Transform>().Get(entityID).rotation;
}

void TransformSystem::SetScale(const uint32_t entityID, const float x, const float y, const float z) const {
	auto &transform = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.scale = Math::Vec3(x, y, z);
	transform.state = Components::Transform::TransformState::Dirty;
}

void TransformSystem::SetScale(const uint32_t entityID, const Math::Vec3 scale) const {
	auto &transform = ref_eM->GetCompArr<Components::Transform>().Get(entityID);
	transform.scale = scale;
	transform.state = Components::Transform::TransformState::Dirty;
}

Math::Vec3 TransformSystem::GetScale(const uint32_t entityID) const {
	return ref_eM->GetCompArr<Components::Transform>().Get(entityID).scale;
}

void TransformSystem::UpdateWorldMatrix(Components::Transform &transform) {
	const Math::Mat44 identity	  = Math::Mat44Identity();
	const Math::Mat44 scale		  = Math::Scale(identity, transform.scale);
	Math::Mat44		  rotation	  = Math::Rotate(identity, transform.rotation.y, Math::Vec3(0.0f, 1.0f, 0.0f));
	rotation					  = Math::Rotate(rotation, transform.rotation.x, Math::Vec3(1.0f, 0.0f, 0.0f));
	rotation					  = Math::Rotate(rotation, transform.rotation.z, Math::Vec3(0.0f, 0.0f, 1.0f));
	const Math::Mat44 translation = Math::Translate(identity, transform.position);

	transform.localMatrix = translation * rotation * scale;
	transform.worldMatrix = transform.localMatrix;
}

void TransformSystem::UpdateWorldMatrix(Components::Transform		&transform,
										const Components::Transform &parentTransform) {
	const Math::Mat44 identity	  = Math::Mat44Identity();
	const Math::Mat44 scale		  = Math::Scale(identity, transform.scale);
	Math::Mat44		  rotation	  = Math::Rotate(identity, transform.rotation.y, Math::Vec3(0.0f, 1.0f, 0.0f));
	rotation					  = Math::Rotate(rotation, transform.rotation.x, Math::Vec3(1.0f, 0.0f, 0.0f));
	rotation					  = Math::Rotate(rotation, transform.rotation.z, Math::Vec3(0.0f, 0.0f, 1.0f));
	const Math::Mat44 translation = Math::Translate(identity, transform.position);

	transform.localMatrix = translation * rotation * scale;
	transform.worldMatrix = parentTransform.worldMatrix * transform.localMatrix;
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

	uint32_t myNewPackedIndex = (uint32_t)sortedData.size();

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