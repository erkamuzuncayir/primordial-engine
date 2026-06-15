#pragma once
#include <cstdint>

namespace PE::ECS {

using EntityID		   = uint32_t;	// Bit-packed full ID (Index + Generation)
using EntityIndex	   = uint32_t;	// Pure array index (0, 1, 2... maxEntityCount)
using EntityGeneration = uint32_t;	// Pure generation/version number (1, 2, 3...)

constexpr EntityID INVALID_ENTITY_ID = UINT32_MAX;

constexpr uint32_t ENTITY_INDEX_BITS	  = 20;
constexpr uint32_t ENTITY_INDEX_MASK	  = (1 << ENTITY_INDEX_BITS) - 1;  // 0x000FFFFF
constexpr uint32_t ENTITY_GENERATION_MASK = ~ENTITY_INDEX_MASK;			   // 0xFFF00000

inline EntityIndex GetEntityIndex(const EntityID id) { return id & ENTITY_INDEX_MASK; }

inline EntityGeneration GetEntityGeneration(const EntityID id) { return (id & ENTITY_GENERATION_MASK) >> ENTITY_INDEX_BITS; }

inline EntityID CreateEntityID(const uint32_t index, const uint32_t generation) {
	return (index & ENTITY_INDEX_MASK) | (generation << ENTITY_INDEX_BITS);
}

struct Entity {
	bool Initialize(EntityID initialId);
	void Shutdown();

	EntityID id = INVALID_ENTITY_ID;
};
}  // namespace PE::ECS