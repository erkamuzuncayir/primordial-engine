#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <stack>
#include <vector>

#include "ComponentArray.h"
#include "ComponentType.h"
#include "Entity.h"
#include "ISystem.h"

namespace PE::ECS {
class EntityManager {
public:
	EntityManager()									= default;
	EntityManager(const EntityManager &)			= delete;
	EntityManager &operator=(const EntityManager &) = delete;
	EntityManager(EntityManager &&)					= delete;
	EntityManager &operator=(EntityManager &&)		= delete;
	~EntityManager()								= default;

	// Initialize with maximum number of entities and component types
	ERROR_CODE Initialize(uint32_t maxEntities, uint32_t maxComponentTypes);
	void	   Update(float dt);
	ERROR_CODE Shutdown();

	// Entity lifecycle
	EntityID   CreateEntity();
	ERROR_CODE DestroyEntity(EntityID id);
	void	   ClearAllEntities();

	// Component registration + access
	template <typename TIComponent>
	ERROR_CODE RegisterComponent(uint32_t componentCount = 0);
	template <typename TIComponent>
	ERROR_CODE UnregisterComponent();
	template <typename TIComponent>
	ERROR_CODE AddComponent(const EntityID entityID, const TIComponent &component);
	template <typename TIComponent>
	ERROR_CODE RemoveComponent(const EntityID entityID);
	template <typename TIComponent>
	[[nodiscard]] bool HasComponent(const EntityID entityID) const;
	template <typename TIComponent>
	TIComponent *GetTIComponent(const EntityID entityID);
	template <class TIComponent>
	TIComponent *TryGetTIComponent(EntityID entityID);
	template <typename... TIComponents>
	std::tuple<TIComponents *...> GetTIComponents(const EntityID entityID);

	// System registration + update
	ERROR_CODE RegisterSystem(ISystem *system);
	ERROR_CODE UnregisterSystem(const ISystem *system);
	template <class T>
	ComponentArray<T> &GetCompArr();

private:
	SystemState			 m_state = SystemState::Uninitialized;
	uint32_t			 ref_maxEntities{0};
	uint32_t			 ref_maxComponentTypes{0};
	std::stack<EntityID> m_freeEntities;  // recycled IDs

	// Flat mapping: [typeID * ref_maxEntities + entityID] -> componentIndex or UINT32_MAX
	std::vector<uint32_t>						  m_allComponentIndices;
	std::vector<std::unique_ptr<IComponentArray>> m_componentArrays;

	// Game systems (ordered by stage)
	std::array<std::vector<ISystem *>, static_cast<size_t>(ESystemStage::Count)> m_systems;
};

// Template implementations
template <typename T>
ComponentArray<T> &EntityManager::GetCompArr() {
	const uint32_t typeID = ComponentType<T>::ID();
	return *static_cast<ComponentArray<T> *>(m_componentArrays[typeID].get());
}

template <typename TIComponent>
ERROR_CODE EntityManager::RegisterComponent(uint32_t componentCount) {
	const uint32_t typeID = ComponentType<TIComponent>::ID();
	if (typeID >= ref_maxComponentTypes) {
		// TODO: Add reallocation for increased size of m_allComponentIndices
		PE_LOG_ERROR("Too many component types. Consider increasing ref_maxComponentTypes.");
		return ERROR_CODE::MAX_COMPONENT_TYPES_REACHED;
	}

	if (m_componentArrays[typeID]) {
		PE_LOG_FATAL("Component is already registered!");
		return ERROR_CODE::COMPONENT_ALREADY_REGISTERED;
	}

	auto array = std::make_unique<ComponentArray<TIComponent>>();
	array->Initialize(componentCount == 0 ? ref_maxEntities : componentCount);
	m_componentArrays[typeID] = std::move(array);

	return ERROR_CODE::OK;
}

template <typename TIComponent>
ERROR_CODE EntityManager::UnregisterComponent() {
	const uint32_t typeID = ComponentType<TIComponent>::ID();
	if (typeID >= ref_maxComponentTypes) {
		PE_LOG_FATAL("Invalid component type");
		return ERROR_CODE::INVALID_COMPONENT_TYPE;
	}

	if (!m_componentArrays[typeID]) {
		PE_LOG_FATAL("Component not registered");
		return ERROR_CODE::COMPONENT_NOT_REGISTERED;
	}

	m_componentArrays[typeID]->Shutdown();
	m_componentArrays[typeID] = nullptr;
	return ERROR_CODE::OK;
}

template <typename TIComponent>
ERROR_CODE EntityManager::AddComponent(EntityID entityID, const TIComponent &component) {
	if (entityID >= ref_maxEntities) {
		PE_LOG_FATAL("Wrong entity ID.");
		return ERROR_CODE::WRONG_ENTITY_ID;
	}

	const uint32_t typeID = ComponentType<TIComponent>::ID();
	auto		  &array  = static_cast<ComponentArray<TIComponent> &>(*m_componentArrays[typeID]);
	const uint32_t idx	  = array.Add(entityID, &component);

	m_allComponentIndices[typeID * ref_maxEntities + entityID] = idx;

	return ERROR_CODE::OK;
}

template <typename TIComponent>
ERROR_CODE EntityManager::RemoveComponent(const EntityID entityID) {
	if (entityID >= ref_maxEntities) {
		PE_LOG_FATAL("Wrong entity ID.");
		return ERROR_CODE::WRONG_ENTITY_ID;
	}

	const uint32_t typeID = ComponentType<TIComponent>::ID();
	const int32_t  idx	  = m_allComponentIndices[typeID * ref_maxEntities + entityID];

	if (idx == UINT32_MAX) {
		PE_LOG_WARN("Component ID is default value.");
		return ERROR_CODE::COMPONENT_IS_IN_DEFAULT_STATE;
	}

	const auto [movedSlot, newPackedIdx] = m_componentArrays[typeID]->Remove(idx);

	// Clear the removed entity's mapping
	m_allComponentIndices[typeID * ref_maxEntities + entityID] = UINT32_MAX;

	// Update the mapping for the moved slot/entity
	m_allComponentIndices[typeID * ref_maxEntities + movedSlot] = newPackedIdx;

	return ERROR_CODE::OK;
}

template <typename TIComponent>
TIComponent *EntityManager::GetTIComponent(const EntityID entityID) {
	if (entityID >= ref_maxEntities) {
		PE_LOG_FATAL("Wrong entity ID.");
		return nullptr;
	}

	const uint32_t typeID = ComponentType<TIComponent>::ID();
	const uint32_t idx	  = m_allComponentIndices[typeID * ref_maxEntities + entityID];

	if (idx == UINT32_MAX) {
		PE_LOG_FATAL("Entity doesn't have the component.");
		return nullptr;
	}

	IComponentArray *arrBase = m_componentArrays[typeID].get();

	if (!arrBase) {
		PE_LOG_FATAL("Component array pointer is null, but index map was valid!");
		return nullptr;
	}

	if (!arrBase->Has(idx)) {
		PE_LOG_FATAL("Entity idx exists, but array::Has(idx) returned false.");
		return nullptr;
	}

	auto componentArray = static_cast<ComponentArray<TIComponent> *>(arrBase);
	return &(componentArray->Get(idx));
}

template <typename TIComponent>
TIComponent *EntityManager::TryGetTIComponent(EntityID entityID) {
	if (HasComponent<TIComponent>(entityID))
		return GetTIComponent<TIComponent>(entityID);
	else {
		PE_LOG_TRACE("Entity does not have the component.");
		return nullptr;
	}
}

template <typename... TIComponents>
std::tuple<TIComponents *...> EntityManager::GetTIComponents(EntityID entityID) {
	return std::tuple<TIComponents *...>{GetTIComponent<TIComponents>(entityID)...};
}

template <typename TIComponent>
bool EntityManager::HasComponent(const EntityID entityID) const {
	if (entityID >= ref_maxEntities) {
		PE_LOG_WARN("Wrong entity ID.");
		return false;
	}

	const uint32_t typeID = ComponentType<TIComponent>::ID();
	return m_allComponentIndices[typeID * ref_maxEntities + entityID] != UINT32_MAX;
}
}  // namespace PE::ECS