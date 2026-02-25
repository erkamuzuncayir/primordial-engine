#pragma once

#include "../Components/Transform.h"
#include "Core/EngineConfig.h"
#include "ECS/EntityManager.h"
#include "ECS/ISystem.h"
#include "Graphics/Systems/CameraSystem.h"
#include "Input/InputSystem.h"
#include "Math/Math.h"

namespace PE::Scene::Systems {
class TransformSystem : public ECS::ISystem {
public:
	TransformSystem()									= default;
	TransformSystem(const TransformSystem &)			= delete;
	TransformSystem &operator=(const TransformSystem &) = delete;
	TransformSystem(TransformSystem &&)					= delete;
	TransformSystem &operator=(TransformSystem &&)		= delete;
	~TransformSystem() override							= default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::EntityManager *entityManager, Input::InputSystem *inputSystem,
						  Graphics::Systems::CameraSystem *cameraSystem, const Core::EngineConfig &config);
	ERROR_CODE Shutdown() override;

	void OnUpdate(float dt) override;

	void					 SetPosition(uint32_t entityID, float x, float y, float z) const;
	void					 SetPosition(uint32_t entityID, Math::Vec3 pos) const;
	[[nodiscard]] Math::Vec3 GetPosition(uint32_t entityID) const;

	void					 SetRotation(uint32_t entityID, float pitch, float yaw, float roll) const;	// in radians
	void					 SetRotation(uint32_t entityID, Math::Vec3 rot) const;						// in radians
	[[nodiscard]] Math::Vec3 GetRotation(uint32_t entityID) const;

	void					 SetScale(uint32_t entityID, float x, float y, float z) const;
	void					 SetScale(uint32_t entityID, Math::Vec3 scale) const;
	[[nodiscard]] Math::Vec3 GetScale(uint32_t entityID) const;

	void UpdateWorldMatrix(Components::Transform &transform);
	void UpdateWorldMatrix(Components::Transform &transform, const Components::Transform &parentTransform);

	void AttachEntity(uint32_t childEntityID, uint32_t parentEntityID);
	void DetachEntity(uint32_t entityID);

	void MarkDirty() { m_isHierarchyDirty = true; }

private:
	void RebuildTransformArray();
	void DFSRebuild(uint32_t entityID, uint32_t currentParentPackedIndex, const std::vector<std::vector<uint32_t>> &adj,
					std::vector<Components::Transform> &sortedData, std::vector<uint32_t> &sortedEntities);

	ECS::EntityManager		 *ref_eM	 = nullptr;
	const Core::EngineConfig *ref_config = nullptr;

	bool	 m_isHierarchyDirty	  = true;
	uint32_t m_lastComponentCount = 0;
};
}  // namespace PE::Scene::Systems