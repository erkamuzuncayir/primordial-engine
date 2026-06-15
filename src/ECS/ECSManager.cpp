#include "ECS/ECSManager.h"

#include "Scene/EntityFactory.h"
#include "Utilities/MemoryUtilities.h"

namespace PE::ECS {
ERROR_CODE ECSManager::Initialize(const Core::EngineConfig &config) {
	PE_CHECK_STATE_INIT(m_state, "Entity manager is already initialized");
	m_state = SystemState::Initializing;

	ref_config = &config;
	m_allComponentIndices.assign(ref_config->maxEntityCount * ref_config->maxComponentTypeCount, UINT32_MAX);
	m_componentArrays.clear();
	m_componentArrays.resize(ref_config->maxComponentTypeCount);

	m_entityGenerations.assign(ref_config->maxEntityCount, 1);

	while (!m_freeEntities.empty()) m_freeEntities.pop();
	for (EntityIndex i = 0; i < ref_config->maxEntityCount; ++i)
		m_freeEntities.push(ref_config->maxEntityCount - 1 - i);

	ERROR_CODE result = ERROR_CODE::OK;
	PE_CHECK(result, Scene::EntityFactory::Initialize(this));

	m_state = SystemState::Running;
	return result;
}

void ECSManager::Update(const float dt) const {
	for (auto const &stageVec : m_systems)
		for (auto &sys : stageVec) sys->OnUpdate(dt);
}

ERROR_CODE ECSManager::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	for (int i = static_cast<int>(ESystemStage::Count) - 1; i >= 0; --i) {
		for (const auto &sys : m_systems[i])
			if (sys) sys->Shutdown();
	}

	m_allComponentIndices.clear();
	m_componentArrays.clear();
	Scene::EntityFactory::Shutdown();
	m_state = SystemState::Uninitialized;
	return ERROR_CODE::OK;
}

EntityID ECSManager::CreateEntity() {
	if (m_freeEntities.empty()) {
		PE_LOG_ERROR("There isn't any free entity.");
		return INVALID_ENTITY_ID;
	}

	const auto index = m_freeEntities.top();
	m_freeEntities.pop();

	for (uint32_t typeID = 0; typeID < ref_config->maxComponentTypeCount; ++typeID)
		m_allComponentIndices[typeID * ref_config->maxEntityCount + index] = INVALID_ENTITY_ID;

	return CreateEntityID(index, m_entityGenerations[index]);
}

ERROR_CODE ECSManager::DestroyEntity(const EntityID id) {
	const uint32_t index	  = GetEntityIndex(id);
	const uint32_t generation = GetEntityGeneration(id);

	if (index >= ref_config->maxEntityCount) {
		PE_LOG_FATAL("Entity ID index isn't correct.");
		return ERROR_CODE::WRONG_ENTITY_ID;
	}

	if (m_entityGenerations[index] != generation) {
		PE_LOG_WARN("Attempted to destroy an already destroyed stale entity reference.");
		return ERROR_CODE::WRONG_ENTITY_ID;
	}

	for (uint32_t typeID = 0; typeID < ref_config->maxComponentTypeCount; ++typeID) {
		if (const uint32_t idx = m_allComponentIndices[typeID * ref_config->maxEntityCount + index];
			idx != UINT32_MAX && m_componentArrays[typeID]->Has(id)) {
			m_componentArrays[typeID]->Remove(id);
			m_allComponentIndices[typeID * ref_config->maxEntityCount + index] = UINT32_MAX;
		}
	}

	m_entityGenerations[index] = (m_entityGenerations[index] + 1) & (ENTITY_GENERATION_MASK >> ENTITY_INDEX_BITS);

	m_freeEntities.push(index);
	return ERROR_CODE::OK;
}

void ECSManager::ClearAllEntities() {
	PE_LOG_INFO("ECSManager: Clearing all entities and components...");

	for (uint32_t typeID = 0; typeID < ref_config->maxComponentTypeCount; ++typeID) {
		if (m_componentArrays[typeID]) { m_componentArrays[typeID]->Clear(); }
	}

	std::fill(m_allComponentIndices.begin(), m_allComponentIndices.end(), UINT32_MAX);

	while (!m_freeEntities.empty()) m_freeEntities.pop();

	for (EntityID i = 0; i < ref_config->maxEntityCount; ++i) {
		m_freeEntities.push(ref_config->maxEntityCount - 1 - i);
	}
	m_entityGenerations.assign(ref_config->maxEntityCount, 1);

	PE_LOG_INFO("ECSManager: All entities cleared.");
}

ERROR_CODE ECSManager::RegisterSystem(ISystem *system) {
	ESystemStage stage = system->GetStage();

	if (stage >= ESystemStage::Count) {
		PE_LOG_FATAL("Invalid system stage.");
		return ERROR_CODE::SYSTEM_INVALID_STAGE;
	}

	for (const auto &existing : m_systems[static_cast<size_t>(stage)]) {
		if (existing->GetID() == system->GetID()) {
			PE_LOG_FATAL("System already registered in this stage");
			return ERROR_CODE::SYSTEM_ALREADY_REGISTERED;
		}
	}
	m_systems[static_cast<size_t>(stage)].emplace_back(system);

	return ERROR_CODE::OK;
}

ERROR_CODE ECSManager::UnregisterSystem(const ISystem *system) {
	ESystemStage stage = system->GetStage();

	if (stage >= ESystemStage::Count) {
		PE_LOG_FATAL("Invalid system stage.");
		return ERROR_CODE::SYSTEM_INVALID_STAGE;
	}

	for (auto it = m_systems[static_cast<size_t>(stage)].begin(); it != m_systems[static_cast<size_t>(stage)].end();
		 ++it) {
		if ((*it)->GetID() == system->GetID()) {
			m_systems[static_cast<size_t>(stage)].erase(it);
			return ERROR_CODE::OK;
		}
	}

	PE_LOG_FATAL("System not found in its stage");
	return ERROR_CODE::SYSTEM_NOT_REGISTERED;
}
}  // namespace PE::ECS