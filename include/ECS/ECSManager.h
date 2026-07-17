#pragma once

#include <array>
#include <cstdint>
#include <format>
#include <memory>
#include <stack>
#include <vector>
#include <typeinfo>

#include "ComponentArray.h"
#include "ComponentType.h"
#include "Core/EngineConfig.h"
#include "Entity.h"
#include "ISystem.h"

namespace PE::ECS {
class ECSManager {
public:
	ECSManager()							  = default;
	ECSManager(const ECSManager &)			  = delete;
	ECSManager &operator=(const ECSManager &) = delete;
	ECSManager(ECSManager &&)				  = delete;
	ECSManager &operator=(ECSManager &&)	  = delete;
	~ECSManager()							  = default;

	// Initialize with maximum number of entities and component types
	ERROR_CODE Initialize(const Core::EngineConfig &config);
	void	   Update(float dt);
	ERROR_CODE Shutdown();

	// Entity lifecycle
	EntityID   CreateEntity();
	ERROR_CODE DestroyEntity(EntityID id);
	void	   ClearAllEntities();

	// Component registration + access
	template <typename TComponent>
	ERROR_CODE RegisterComponent(uint32_t componentCount = 0);
	template <typename TComponent>
	ERROR_CODE UnregisterComponent();
	template <typename TComponent>
	ERROR_CODE AddComponent(EntityID entityID, const TComponent &component);
	template <typename TComponent>
	ERROR_CODE RemoveComponent(EntityID entityID);
	template <typename TComponent>
	[[nodiscard]] bool HasComponent(EntityID entityID) const;
	template <typename TComponent>
	TComponent *GetTComponent(EntityID entityID);
	template <class TComponent>
	TComponent *TryGetTComponent(EntityID entityID);
	template <typename... TComponents>
	std::tuple<TComponents *...> GetTComponents(EntityID entityID);

	// System registration + update
	ERROR_CODE RegisterSystem(ISystem *system);
	ERROR_CODE UnregisterSystem(const ISystem *system);
	template <class T>
	ComponentArray<T> &GetCompArr();

private:
	SystemState				  m_state	 = SystemState::Uninitialized;
	const Core::EngineConfig *ref_config = nullptr;
	std::stack<EntityID>	  m_freeEntities;  // recycled IDs

	// Flat mapping: [typeID * ref_config->maxEntityCount + entityID] -> componentIndex or UINT32_MAX
	std::vector<uint32_t>						  m_allComponentIndices;
	std::vector<std::unique_ptr<IComponentArray>> m_componentArrays;

	// Game systems (ordered by stage)
	std::array<std::vector<ISystem *>, static_cast<size_t>(ESystemStage::Count)> m_systems;
};

// Template implementations
template <typename T>
ComponentArray<T> &ECSManager::GetCompArr() {
	const uint32_t typeID = ComponentType<T>::ID();
	return *static_cast<ComponentArray<T> *>(m_componentArrays[typeID].get());
}

template <typename TComponent>
ERROR_CODE ECSManager::RegisterComponent(uint32_t componentCount) {
	const uint32_t typeID = ComponentType<TComponent>::ID();
	if (typeID >= ref_config->maxComponentTypeCount) {
		// TODO: Add reallocation for increased size of m_allComponentIndices
		PE_LOG_ERROR("Too many component types. Consider increasing ref_config->maxComponentTypeCount.");
		return ERROR_CODE::MAX_COMPONENT_TYPES_REACHED;
	}

	if (m_componentArrays[typeID]) {
		PE_LOG_FATAL("Component is already registered!");
		return ERROR_CODE::COMPONENT_ALREADY_REGISTERED;
	}

	auto array = std::make_unique<ComponentArray<TComponent>>();
	array->Initialize(componentCount == 0 ? ref_config->maxEntityCount : componentCount);
	m_componentArrays[typeID] = std::move(array);

	return ERROR_CODE::OK;
}

template <typename TComponent>
ERROR_CODE ECSManager::UnregisterComponent() {
	const uint32_t typeID = ComponentType<TComponent>::ID();
	if (typeID >= ref_config->maxComponentTypeCount) {
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

template <typename TComponent>
ERROR_CODE ECSManager::AddComponent(EntityID entityID, const TComponent &component) {
	if (entityID >= ref_config->maxEntityCount) {
		PE_LOG_FATAL("Wrong entity ID.");
		return ERROR_CODE::WRONG_ENTITY_ID;
	}

	const uint32_t typeID = ComponentType<TComponent>::ID();
	auto		  &array  = static_cast<ComponentArray<TComponent> &>(*m_componentArrays[typeID]);
	const uint32_t idx	  = array.Add(entityID, &component);

	m_allComponentIndices[typeID * ref_config->maxEntityCount + entityID] = idx;

	return ERROR_CODE::OK;
}

template <typename TComponent>
ERROR_CODE ECSManager::RemoveComponent(const EntityID entityID) {
	if (entityID >= ref_config->maxEntityCount) {
		PE_LOG_FATAL("Wrong entity ID.");
		return ERROR_CODE::WRONG_ENTITY_ID;
	}

	const uint32_t typeID = ComponentType<TComponent>::ID();
	const int32_t  idx	  = m_allComponentIndices[typeID * ref_config->maxEntityCount + entityID];

	if (idx == UINT32_MAX) {
		PE_LOG_WARN("Component ID is default value.");
		return ERROR_CODE::COMPONENT_IS_IN_DEFAULT_STATE;
	}

	m_componentArrays[typeID]->Remove(idx);

	// Clear the removed entity's mapping
	m_allComponentIndices[typeID * ref_config->maxEntityCount + entityID] = UINT32_MAX;

	return ERROR_CODE::OK;
}

template <typename TComponent>
TComponent *ECSManager::GetTComponent(const EntityID entityID) {
	if (entityID >= ref_config->maxEntityCount) {
		PE_LOG_FATAL("Wrong entity ID.");
		return nullptr;
	}

	const uint32_t typeID = ComponentType<TComponent>::ID();
	const uint32_t idx	  = m_allComponentIndices[typeID * ref_config->maxEntityCount + entityID];

	if (idx == UINT32_MAX) {
		PE_LOG_FATAL(std::format("Entity {} does not have the requested component: {}", entityID,
					 typeid(TComponent).name()));
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

	auto componentArray = static_cast<ComponentArray<TComponent> *>(arrBase);
	return &(componentArray->Get(idx));
}

template <typename TComponent>
TComponent *ECSManager::TryGetTComponent(EntityID entityID) {
	if (HasComponent<TComponent>(entityID))
		return GetTComponent<TComponent>(entityID);
	else {
		PE_LOG_TRACE("Entity does not have the component.");
		return nullptr;
	}
}

template <typename... TComponents>
std::tuple<TComponents *...> ECSManager::GetTComponents(EntityID entityID) {
	return std::tuple<TComponents *...>{GetTComponent<TComponents>(entityID)...};
}

template <typename TComponent>
bool ECSManager::HasComponent(const EntityID entityID) const {
	if (entityID >= ref_config->maxEntityCount) {
		PE_LOG_WARN("Wrong entity ID.");
		return false;
	}

	const uint32_t typeID = ComponentType<TComponent>::ID();
	return m_allComponentIndices[typeID * ref_config->maxEntityCount + entityID] != UINT32_MAX;
}
}  // namespace PE::ECS