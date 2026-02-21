#pragma once

#include <cstdint>

namespace PE::ECS {
using EntityID						 = uint32_t;
constexpr uint32_t INVALID_ENTITY_ID = UINT32_MAX;

struct Entity {
	bool Initialize(uint32_t);
	void Shutdown();

	uint32_t id = INVALID_ENTITY_ID;
};
}  // namespace PE::ECS