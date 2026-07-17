#pragma once

#include "../Components/Transform.h"
#include "Core/EngineConfig.h"
#include "ECS/ECSManager.h"
#include "ECS/ISystem.h"
#include "Math/Math.h"

namespace PE::Physics::Core::Systems {
class PhysicsSystem;
}

namespace PE::Scene::Systems {
class TransformSystem : public ECS::ISystem {
public:
	TransformSystem()									= default;
	TransformSystem(const TransformSystem &)			= delete;
	TransformSystem &operator=(const TransformSystem &) = delete;
	TransformSystem(TransformSystem &&)					= delete;
	TransformSystem &operator=(TransformSystem &&)		= delete;
	~TransformSystem() override							= default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::ECSManager *ecsManager,
						  const Physics::Core::Systems::PhysicsSystem &physicsSystem, const Core::EngineConfig &config);
	ERROR_CODE Shutdown() override;

	void OnUpdate(float dt) override;

	void ResetSyncData();
	void					 SetPosition(uint32_t entityID, float x, float y, float z) const;
	void					 SetPosition(uint32_t entityID, Math::Vec3 pos) const;
	[[nodiscard]] Math::Vec3 GetPosition(uint32_t entityID) const;
	void					 SyncPosition(uint32_t entityID, float x, float y, float z) const;
	void					 SyncPosition(uint32_t entityID, Math::Vec3 pos) const;

	void SyncPositionAndOrientation(uint32_t entityID, Math::Vec3 pos, Math::Quat orientation) const;

	void					 SetOrientation(uint32_t entityID, float pitch, float yaw, float roll) const;  // in radians
	void					 SetOrientation(uint32_t entityID, Math::Quat rot) const;
	void					 SetOrientation(uint32_t entityID, Math::Vec3 radOrientation) const;  // in radians
	[[nodiscard]] Math::Quat GetOrientation(uint32_t entityID) const;

	void					 SetScale(uint32_t entityID, float x, float y, float z) const;
	void					 SetScale(uint32_t entityID, Math::Vec3 scale) const;
	[[nodiscard]] Math::Vec3 GetScale(uint32_t entityID) const;

	void SetPositionAndOrientation(uint32_t entityID, Math::Vec3 position, Math::Quat orientation) const;

	[[nodiscard]] std::vector<ECS::EntityID> &GetUpdatedTransformEntityIDs() { return m_updatedTransformEntityIDs; }

	void ProcessDirtyTransform(ECS::EntityID entityId, Components::Transform &transform);
	void ProcessDirtyTransform(ECS::EntityID entityId, Components::Transform &transform,
							   Components::Transform const &parent);

	void UpdateWorldMatrix(Components::Transform &transform);
	void UpdateWorldMatrix(Components::Transform &transform, const Components::Transform &parentTransform);

	void AttachEntity(uint32_t childEntityID, uint32_t parentEntityID);
	void DetachEntity(uint32_t entityID);

	void MarkDirty() { m_isHierarchyDirty = true; }

private:
	void RebuildTransformArray();
	void DFSRebuild(uint32_t entityID, uint32_t currentParentPackedIndex, const std::vector<std::vector<uint32_t>> &adj,
					std::vector<Components::Transform> &sortedData, std::vector<uint32_t> &sortedEntities);

	ECS::ECSManager								*ref_eM			   = nullptr;
	const Core::EngineConfig					*ref_config		   = nullptr;
	const Physics::Core::Systems::PhysicsSystem *ref_physicsSystem = nullptr;

	std::vector<ECS::EntityID> m_updatedTransformEntityIDs;
	bool					   m_isHierarchyDirty	= true;
	uint32_t				   m_lastComponentCount = 0;
};
}  // namespace PE::Scene::Systems