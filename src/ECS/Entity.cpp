#include "ECS/Entity.h"

namespace PE::ECS {
bool Entity::Initialize(const EntityID initialId) {
	id = initialId;
	return true;
}

void Entity::Shutdown() { id = INVALID_ENTITY_ID; }
}  // namespace PE::ECS