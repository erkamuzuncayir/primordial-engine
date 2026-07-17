#pragma once
#include "ECS/ECSManager.h"
#include "Physics/Body/Components/AABB.h"

namespace PE::Physics::Body::Systems {
static constexpr Components::ColliderShape SHAPE_COUNT = Components::ColliderShape::Count;

struct SpatialEntry {
	ECS::EntityID entityID;
	uint32_t	  cellHash;
};

struct CollisionPair {
	// Dynamic is always first one!
	ECS::EntityID entityIdOne;
	ECS::EntityID entityIdTwo;
};

struct CollisionPairs {
	const std::vector<CollisionPair> (
		&dynamicVsStatic)[static_cast<uint32_t>(SHAPE_COUNT)][static_cast<uint32_t>(SHAPE_COUNT)];
	const std::vector<CollisionPair> (
		&dynamicVsKinematic)[static_cast<uint32_t>(SHAPE_COUNT)][static_cast<uint32_t>(SHAPE_COUNT)];
	const std::vector<CollisionPair> (
		&dynamicVsDynamic)[static_cast<uint32_t>(SHAPE_COUNT)][static_cast<uint32_t>(SHAPE_COUNT)];
	const std::vector<CollisionPair> (
		&dynSolidVsStatCont)[static_cast<uint32_t>(SHAPE_COUNT)][static_cast<uint32_t>(SHAPE_COUNT)];
	const std::vector<CollisionPair> (
		&dynContVsStatSolid)[static_cast<uint32_t>(SHAPE_COUNT)][static_cast<uint32_t>(SHAPE_COUNT)];
	const std::vector<CollisionPair> (
		&dynSolidVsKinCont)[static_cast<uint32_t>(SHAPE_COUNT)][static_cast<uint32_t>(SHAPE_COUNT)];
	const std::vector<CollisionPair> (
		&dynContVsKinSolid)[static_cast<uint32_t>(SHAPE_COUNT)][static_cast<uint32_t>(SHAPE_COUNT)];
	const std::vector<CollisionPair> (
		&dynSolidVsDynCont)[static_cast<uint32_t>(SHAPE_COUNT)][static_cast<uint32_t>(SHAPE_COUNT)];
};

class BroadPhaseCollisionSystem {
public:
	explicit BroadPhaseCollisionSystem()									= default;
	BroadPhaseCollisionSystem(const BroadPhaseCollisionSystem &)			= delete;
	BroadPhaseCollisionSystem &operator=(const BroadPhaseCollisionSystem &) = delete;
	BroadPhaseCollisionSystem(BroadPhaseCollisionSystem &&)					= delete;
	BroadPhaseCollisionSystem &operator=(BroadPhaseCollisionSystem &&)		= delete;
	~BroadPhaseCollisionSystem()											= default;

	void Initialize(ECS::ECSManager *ecsManager);
	void Shutdown();
	void OnUpdate();

	CollisionPairs &GetCollisionPairs() { return m_collisionPairs; }

private:
	void BuildStaticSpatialGrid();

	void BuildKinematicSpatialGrid();

	void	 BuildDynamicSpatialGrid();
	[[nodiscard]] uint32_t CalculateHash(uint32_t cellX, uint32_t cellZ) const;
	void	 CheckDynamicVsStatic();
	void	 CheckDynamicVsKinematic();
	void	 CheckDynamicVsDynamic();
	bool	 TestAABB(const Components::AABB &a, const Components::AABB &b);

	ECS::ECSManager			 *ref_eM = nullptr;
	std::vector<SpatialEntry> m_staticGrid;
	std::vector<SpatialEntry> m_kinematicGrid;
	std::vector<SpatialEntry> m_dynamicGrid;

	std::vector<CollisionPair> m_dynamicVsStatic[static_cast<uint32_t>(SHAPE_COUNT)]
												[static_cast<uint32_t>(SHAPE_COUNT)];
	std::vector<CollisionPair> m_dynamicVsKinematic[static_cast<uint32_t>(SHAPE_COUNT)]
												   [static_cast<uint32_t>(SHAPE_COUNT)];
	std::vector<CollisionPair> m_dynamicVsDynamic[static_cast<uint32_t>(SHAPE_COUNT)]
												 [static_cast<uint32_t>(SHAPE_COUNT)];
	std::vector<CollisionPair> m_dynSolidVsStatCont[static_cast<uint32_t>(SHAPE_COUNT)]
												   [static_cast<uint32_t>(SHAPE_COUNT)];
	std::vector<CollisionPair> m_dynContVsStatSolid[static_cast<uint32_t>(SHAPE_COUNT)]
												   [static_cast<uint32_t>(SHAPE_COUNT)];
	std::vector<CollisionPair> m_dynSolidVsKinCont[static_cast<uint32_t>(SHAPE_COUNT)]
												  [static_cast<uint32_t>(SHAPE_COUNT)];
	std::vector<CollisionPair> m_dynContVsKinSolid[static_cast<uint32_t>(SHAPE_COUNT)]
												  [static_cast<uint32_t>(SHAPE_COUNT)];
	std::vector<CollisionPair> m_dynSolidVsDynCont[static_cast<uint32_t>(SHAPE_COUNT)]
												  [static_cast<uint32_t>(SHAPE_COUNT)];
	CollisionPairs			   m_collisionPairs{m_dynamicVsStatic,	  m_dynamicVsKinematic, m_dynamicVsDynamic,
												m_dynSolidVsStatCont, m_dynContVsStatSolid, m_dynSolidVsKinCont,
												m_dynContVsKinSolid,  m_dynSolidVsDynCont};
	uint32_t				   m_staticEntityCount = 0;
};
}  // namespace PE::Physics::Body::Systems