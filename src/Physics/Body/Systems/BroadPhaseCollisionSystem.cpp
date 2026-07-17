#include "Physics/Body/Systems/BroadPhaseCollisionSystem.h"

#include <algorithm>
#include <format>

#include "Physics/Body/Components/AABB.h"
#include "Physics/Body/Components/KinematicBody.h"
#include "Physics/Body/Components/RigidBody.h"
#include "Physics/Body/Components/StaticEntity.h"

namespace {
template <typename T>
void RemoveDuplicatePairs(std::vector<T> &vec) {
	std::ranges::sort(vec, [](const auto &a, const auto &b) {
		if (a.entityIdOne != b.entityIdOne) return a.entityIdOne < b.entityIdOne;
		return a.entityIdTwo < b.entityIdTwo;
	});

	auto ret = std::ranges::unique(vec, [](const auto &a, const auto &b) {
		return a.entityIdOne == b.entityIdOne && a.entityIdTwo == b.entityIdTwo;
	});
	vec.erase(ret.begin(), ret.end());
}
}  // namespace

namespace PE::Physics::Body::Systems {
// TODO: Move those to scene config
static constexpr float WORLD_MIN_X = -5000.0f;
static constexpr float WORLD_MIN_Z = -5000.0f;
static constexpr float WORLD_MAX_X = 5000.0f;
static constexpr float WORLD_MAX_Z = 5000.0f;

static constexpr float CELL_SIZE	 = 2.0f;
static constexpr float INV_CELL_SIZE = 1 / CELL_SIZE;

static constexpr uint32_t GRID_WIDTH  = static_cast<uint32_t>((WORLD_MAX_X - WORLD_MIN_X) * INV_CELL_SIZE);
static constexpr uint32_t GRID_HEIGHT = static_cast<uint32_t>((WORLD_MAX_Z - WORLD_MIN_Z) * INV_CELL_SIZE);

void BroadPhaseCollisionSystem::Initialize(ECS::ECSManager *ecsManager) {
	ref_eM = ecsManager;
	// TODO: Reserve space for vectors and get world min and maxes here from scene config?
	BuildStaticSpatialGrid();
}

void BroadPhaseCollisionSystem::Shutdown() {}

void BroadPhaseCollisionSystem::OnUpdate() {
	for (auto &row : m_dynamicVsStatic)
		for (auto &pairs : row) pairs.clear();
	for (auto &row : m_dynamicVsKinematic)
		for (auto &pairs : row) pairs.clear();
	for (auto &row : m_dynamicVsDynamic)
		for (auto &pairs : row) pairs.clear();
	for (auto &row : m_dynSolidVsStatCont)
		for (auto &pairs : row) pairs.clear();
	for (auto &row : m_dynContVsStatSolid)
		for (auto &pairs : row) pairs.clear();
	for (auto &row : m_dynSolidVsKinCont)
		for (auto &pairs : row) pairs.clear();
	for (auto &row : m_dynContVsKinSolid)
		for (auto &pairs : row) pairs.clear();
	for (auto &row : m_dynSolidVsDynCont)
		for (auto &pairs : row) pairs.clear();

	// TODO: Check if GetCount works correctly!
	if (const uint32_t staticEntityCount = ref_eM->GetCompArr<Components::StaticEntity>().GetCount();
		m_staticEntityCount != staticEntityCount) {
		BuildStaticSpatialGrid();
		m_staticEntityCount = staticEntityCount;
	}
	BuildKinematicSpatialGrid();
	BuildDynamicSpatialGrid();
	CheckDynamicVsDynamic();
	CheckDynamicVsKinematic();
	CheckDynamicVsStatic();

	for (auto &row : m_dynamicVsStatic)
		for (auto &pairs : row)
			RemoveDuplicatePairs(pairs);
	for (auto &row : m_dynamicVsKinematic)
		for (auto &pairs : row)
			RemoveDuplicatePairs(pairs);
	for (auto &row : m_dynamicVsDynamic)
		for (auto &pairs : row)
			RemoveDuplicatePairs(pairs);
	for (auto &row : m_dynSolidVsStatCont)
		for (auto &pairs : row)
			RemoveDuplicatePairs(pairs);
	for (auto &row : m_dynContVsStatSolid)
		for (auto &pairs : row)
			RemoveDuplicatePairs(pairs);
	for (auto &row : m_dynSolidVsKinCont)
		for (auto &pairs : row)
			RemoveDuplicatePairs(pairs);
	for (auto &row : m_dynContVsKinSolid)
		for (auto &pairs : row)
			RemoveDuplicatePairs(pairs);
	for (auto &row : m_dynSolidVsDynCont)
		for (auto &pairs : row)
			RemoveDuplicatePairs(pairs);
}

void BroadPhaseCollisionSystem::BuildStaticSpatialGrid() {
	m_staticGrid.clear();
	auto	   &aabbArr	  = ref_eM->GetCompArr<Components::AABB>();
	const auto &staticArr = ref_eM->GetCompArr<Components::StaticEntity>();

	m_staticGrid.reserve(aabbArr.GetCount() * static_cast<size_t>(CELL_SIZE) * 4);
	const auto &aabbs = aabbArr.Data();
	for (uint32_t i = 0; i < aabbs.size(); ++i) {
		const ECS::EntityID entityID = aabbArr.Index()[i];
		if (!staticArr.Has(entityID)) continue;

		const auto &bounds = aabbs[i];
		const int	startX = Math::Clamp(static_cast<int>(std::floor((bounds.min.x - WORLD_MIN_X) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_WIDTH - 1));
		const int	endX   = Math::Clamp(static_cast<int>(std::floor((bounds.max.x - WORLD_MIN_X) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_WIDTH - 1));
		const int	startZ = Math::Clamp(static_cast<int>(std::floor((bounds.min.z - WORLD_MIN_Z) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_HEIGHT - 1));
		const int	endZ   = Math::Clamp(static_cast<int>(std::floor((bounds.max.z - WORLD_MIN_Z) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_HEIGHT - 1));

		for (uint32_t x = startX; x <= endX; ++x) {
			for (uint32_t z = startZ; z <= endZ; ++z)
				m_staticGrid.emplace_back(SpatialEntry{entityID, CalculateHash(x, z)});
		}
	}

	std::ranges::sort(m_staticGrid,
					  [](const SpatialEntry &a, const SpatialEntry &b) { return a.cellHash < b.cellHash; });
}

void BroadPhaseCollisionSystem::BuildKinematicSpatialGrid() {
	m_kinematicGrid.clear();
	auto	   &aabbArr			 = ref_eM->GetCompArr<Components::AABB>();
	const auto &kinematicBodyArr = ref_eM->GetCompArr<Components::KinematicBody>();

	m_kinematicGrid.reserve(aabbArr.GetCount() * static_cast<size_t>(CELL_SIZE) * 4);
	const auto &aabbs = aabbArr.Data();
	for (uint32_t i = 0; i < aabbs.size(); ++i) {
		const ECS::EntityID entityID = aabbArr.Index()[i];
		if (!kinematicBodyArr.Has(entityID)) continue;

		const auto &bounds = aabbs[i];
		const int	startX = Math::Clamp(static_cast<int>(std::floor((bounds.min.x - WORLD_MIN_X) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_WIDTH - 1));
		const int	endX   = Math::Clamp(static_cast<int>(std::floor((bounds.max.x - WORLD_MIN_X) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_WIDTH - 1));
		const int	startZ = Math::Clamp(static_cast<int>(std::floor((bounds.min.z - WORLD_MIN_Z) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_HEIGHT - 1));
		const int	endZ   = Math::Clamp(static_cast<int>(std::floor((bounds.max.z - WORLD_MIN_Z) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_HEIGHT - 1));

		for (uint32_t x = startX; x <= endX; ++x) {
			for (uint32_t z = startZ; z <= endZ; ++z) {
				m_kinematicGrid.emplace_back(SpatialEntry{entityID, CalculateHash(x, z)});
			}
		}
	}

	std::ranges::sort(m_kinematicGrid,
					  [](const SpatialEntry &a, const SpatialEntry &b) { return a.cellHash < b.cellHash; });
}

void BroadPhaseCollisionSystem::BuildDynamicSpatialGrid() {
	m_dynamicGrid.clear();
	auto	   &aabbArr		 = ref_eM->GetCompArr<Components::AABB>();
	const auto &rigidBodyArr = ref_eM->GetCompArr<Components::RigidBody>();

	m_dynamicGrid.reserve(aabbArr.GetCount() * CELL_SIZE * 4);
	const auto &aabbs = aabbArr.Data();
	for (uint32_t i = 0; i < aabbs.size(); ++i) {
		const ECS::EntityID entityID = aabbArr.Index()[i];
		if (!rigidBodyArr.Has(entityID)) continue;

		const auto &bounds = aabbs[i];
		const int	startX = Math::Clamp(static_cast<int>(std::floor((bounds.min.x - WORLD_MIN_X) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_WIDTH - 1));
		const int	endX   = Math::Clamp(static_cast<int>(std::floor((bounds.max.x - WORLD_MIN_X) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_WIDTH - 1));
		const int	startZ = Math::Clamp(static_cast<int>(std::floor((bounds.min.z - WORLD_MIN_Z) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_HEIGHT - 1));
		const int	endZ   = Math::Clamp(static_cast<int>(std::floor((bounds.max.z - WORLD_MIN_Z) / CELL_SIZE)), 0,
										 static_cast<int>(GRID_HEIGHT - 1));

		for (uint32_t x = startX; x <= endX; ++x) {
			for (uint32_t z = startZ; z <= endZ; ++z) {
				m_dynamicGrid.emplace_back(SpatialEntry{entityID, CalculateHash(x, z)});
			}
		}
	}

	std::ranges::sort(m_dynamicGrid,
					  [](const SpatialEntry &a, const SpatialEntry &b) { return a.cellHash < b.cellHash; });
}

inline uint32_t BroadPhaseCollisionSystem::CalculateHash(const uint32_t cellX, const uint32_t cellZ) const {
	// Convert the 2D cell coordinate into a 1D hash index
	return cellX + cellZ * GRID_WIDTH;
}

void BroadPhaseCollisionSystem::CheckDynamicVsStatic() {
	const auto	&aabbArr  = ref_eM->GetCompArr<Components::AABB>();
	const size_t dynCount = m_dynamicGrid.size();

	size_t staticStartIdx = 0;
	for (size_t i = 0; i < dynCount; ++i) {
		const uint32_t		hashA		  = m_dynamicGrid[i].cellHash;
		const ECS::EntityID dynamicEntity = m_dynamicGrid[i].entityID;
		const auto		   &dynamicAABB	  = aabbArr.Get(dynamicEntity);

		while (staticStartIdx < m_staticGrid.size() && m_staticGrid[staticStartIdx].cellHash < hashA) {
			staticStartIdx++;
		}

		for (size_t j = staticStartIdx; j < m_staticGrid.size() && m_staticGrid[j].cellHash == hashA; ++j) {
			const ECS::EntityID staticEntity = m_staticGrid[j].entityID;
			if (dynamicEntity == staticEntity) continue;

			if (const auto &staticAABB = aabbArr.Get(staticEntity); TestAABB(dynamicAABB, staticAABB)) {
				const auto dynamicShape		  = dynamicAABB.GetShapeType();
				const auto isDynamicContainer = dynamicAABB.GetColliderType();
				const auto staticShape		  = staticAABB.GetShapeType();
				const auto isStaticContainer  = staticAABB.GetColliderType();

				// 0: Both Solid
				// 1: Dynamic Solid, Static Container
				// 2: Dynamic Container, Static Solid
				// 3: Both Container
				const uint8_t state =
					(static_cast<uint32_t>(isDynamicContainer) << 1) | static_cast<uint32_t>(isStaticContainer);

				if (state == 0) {
					m_dynamicVsStatic[static_cast<uint32_t>(dynamicShape)][static_cast<uint32_t>(staticShape)]
						.emplace_back(CollisionPair{dynamicEntity, staticEntity});
					continue;
				}
				if (state == 1)
					m_dynSolidVsStatCont[static_cast<uint32_t>(dynamicShape)][static_cast<uint32_t>(staticShape)]
						.emplace_back(CollisionPair{dynamicEntity, staticEntity});
				else if (state == 2)
					m_dynContVsStatSolid[static_cast<uint32_t>(staticShape)][static_cast<uint32_t>(dynamicShape)]
						.emplace_back(CollisionPair{dynamicEntity, staticEntity});
				else {
					if (dynamicAABB.GetVolume() < staticAABB.GetVolume())
						m_dynSolidVsStatCont[static_cast<uint32_t>(dynamicShape)][static_cast<uint32_t>(staticShape)]
							.emplace_back(CollisionPair{dynamicEntity, staticEntity});
					else
						m_dynContVsStatSolid[static_cast<uint32_t>(staticShape)][static_cast<uint32_t>(dynamicShape)]
							.emplace_back(CollisionPair{dynamicEntity, staticEntity});
				}
			}
		}
	}
}

void BroadPhaseCollisionSystem::CheckDynamicVsKinematic() {
	const auto	&aabbArr  = ref_eM->GetCompArr<Components::AABB>();
	const size_t dynCount = m_dynamicGrid.size();

	size_t kinematicStartIdx = 0;
	for (size_t i = 0; i < dynCount; ++i) {
		const uint32_t		hashA		  = m_dynamicGrid[i].cellHash;
		const ECS::EntityID dynamicEntity = m_dynamicGrid[i].entityID;
		const auto		   &dynamicAABB	  = aabbArr.Get(dynamicEntity);

		while (kinematicStartIdx < m_kinematicGrid.size() && m_kinematicGrid[kinematicStartIdx].cellHash < hashA) {
			kinematicStartIdx++;
		}

		for (size_t j = kinematicStartIdx; j < m_kinematicGrid.size() && m_kinematicGrid[j].cellHash == hashA; ++j) {
			const ECS::EntityID kinematicEntity = m_kinematicGrid[j].entityID;
			if (dynamicEntity == kinematicEntity) continue;

			if (const auto &kinematicAABB = aabbArr.Get(kinematicEntity); TestAABB(dynamicAABB, kinematicAABB)) {
				const auto dynamicShape			= dynamicAABB.GetShapeType();
				const auto isDynamicContainer	= dynamicAABB.GetColliderType();
				const auto kinematicShape		= kinematicAABB.GetShapeType();
				const auto isKinematicContainer = kinematicAABB.GetColliderType();

				// 0: Both Solid
				// 1: Dynamic Solid, Kinematic Container
				// 2: Dynamic Container, Kinematic Solid
				// 3: Both Container
				const uint8_t state =
					(static_cast<uint32_t>(isDynamicContainer) << 1) | static_cast<uint32_t>(isKinematicContainer);

				if (state == 0) {
					m_dynamicVsKinematic[static_cast<uint32_t>(dynamicShape)][static_cast<uint32_t>(kinematicShape)]
						.emplace_back(CollisionPair{dynamicEntity, kinematicEntity});
					continue;
				}
				if (state == 1)
					m_dynSolidVsKinCont[static_cast<uint32_t>(dynamicShape)][static_cast<uint32_t>(kinematicShape)]
						.emplace_back(CollisionPair{dynamicEntity, kinematicEntity});
				else if (state == 2)
					m_dynContVsKinSolid[static_cast<uint32_t>(kinematicShape)][static_cast<uint32_t>(dynamicShape)]
						.emplace_back(CollisionPair{dynamicEntity, kinematicEntity});
				else {
					if (dynamicAABB.GetVolume() < kinematicAABB.GetVolume())
						m_dynSolidVsKinCont[static_cast<uint32_t>(dynamicShape)][static_cast<uint32_t>(kinematicShape)]
							.emplace_back(CollisionPair{dynamicEntity, kinematicEntity});
					else
						m_dynContVsKinSolid[static_cast<uint32_t>(kinematicShape)][static_cast<uint32_t>(dynamicShape)]
							.emplace_back(CollisionPair{dynamicEntity, kinematicEntity});
				}
			}
		}
	}
}

void BroadPhaseCollisionSystem::CheckDynamicVsDynamic() {
	const auto &aabbArr = ref_eM->GetCompArr<Components::AABB>();

	const size_t count = m_dynamicGrid.size();

	for (size_t i = 0; i < count; ++i) {
		const uint32_t		hashA	  = m_dynamicGrid[i].cellHash;
		const ECS::EntityID entityOne = m_dynamicGrid[i].entityID;
		const auto		   &aabbOne	  = aabbArr.Get(entityOne);

		for (size_t j = i + 1; j < count; ++j) {
			if (m_dynamicGrid[j].cellHash != hashA) break;

			const ECS::EntityID entityTwo = m_dynamicGrid[j].entityID;
			if (entityOne >= entityTwo) continue;

			if (const auto &aabbTwo = aabbArr.Get(entityTwo); TestAABB(aabbOne, aabbTwo)) {
				const auto shapeOne			   = aabbOne.GetShapeType();
				const auto isShapeOneContainer = aabbOne.GetColliderType();
				const auto shapeTwo			   = aabbTwo.GetShapeType();
				const auto isShapeTwoContainer = aabbTwo.GetColliderType();

				// 0: Both Solid
				// 1: First Solid, Second Container
				// 2: First Container, Second Solid
				// 3: Both Container
				const uint8_t state =
					(static_cast<uint32_t>(isShapeOneContainer) << 1) | static_cast<uint32_t>(isShapeTwoContainer);

				if (!state) {
					// ALWAYS enforce the order: First one Dynamic!
					m_dynamicVsDynamic[static_cast<uint32_t>(shapeOne)][static_cast<uint32_t>(shapeTwo)]
						.emplace_back(CollisionPair{entityOne, entityTwo});
					continue;
				}

				// There are two situations where we need to perform a swap:
				// A) If the first container is the one being swapped (State == 2)
				// B) If both are containers and the first one has a larger capacity (State == 3)
				const bool swap = (state == 2) || (state == 3 && aabbOne.GetVolume() >= aabbTwo.GetVolume());

				const ECS::EntityID entityA = swap ? entityTwo : entityOne;
				const ECS::EntityID entityB = swap ? entityOne : entityTwo;
				const auto			shapeA	= swap ? shapeTwo : shapeOne;
				const auto			shapeB	= swap ? shapeOne : shapeTwo;

				m_dynSolidVsDynCont[static_cast<uint32_t>(shapeA)][static_cast<uint32_t>(shapeB)].emplace_back(
					CollisionPair{entityA, entityB});
			}
		}
	}
}

bool BroadPhaseCollisionSystem::TestAABB(const Components::AABB &a, const Components::AABB &b) {
	const __m128 aMin = _mm_load_ps(&a.min.x);
	const __m128 aMax = _mm_load_ps(&a.max.x);
	const __m128 bMin = _mm_load_ps(&b.min.x);
	const __m128 bMax = _mm_load_ps(&b.max.x);

	// cmp1: a.min <= b.max
	const __m128 cmp1 = _mm_cmple_ps(aMin, bMax);

	// cmp2: a.max >= b.min
	const __m128 cmp2 = _mm_cmpge_ps(aMax, bMin);

	// Combine the comparison results using a bitwise AND
	const __m128 overlap = _mm_and_ps(cmp1, cmp2);

	// Convert the result to an integer mask.
	// If X, Y and Z (the first 3 bits) are 1, the mask will end with 0111 (0x7).
	return (_mm_movemask_ps(overlap) & 0x7) == 0x7;
}
}  // namespace PE::Physics::Body::Systems