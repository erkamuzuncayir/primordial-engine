#include "ECS/Entity.h"

namespace PE::ECS {
bool Entity::Initialize(const uint32_t newId) {
	id = newId;
	return true;
}

void Entity::Shutdown() {
	id = UINT32_MAX;  // UINT32_MAX is reserved id for null
}
}  // namespace PE::ECS