#pragma once
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "Common/Common.h"
#include "IComponentArray.h"
#include "Utilities/Logger.h"

namespace PE::ECS {
using ComponentArrayID = uint32_t;

template <typename T>
class ComponentArray : public IComponentArray {
public:
	ComponentArray() = default;
	~ComponentArray() override;
	ERROR_CODE Initialize(uint32_t maxCount) override;
	ERROR_CODE Shutdown() override;

	uint32_t	Add(uint32_t entityID, const void *componentData) override;
	RemovalInfo Remove(uint32_t entityID) override;

	[[nodiscard]] bool Has(uint32_t entityID) const override;

	// Access methods
	T		&Get(uint32_t entityID);
	const T &Get(uint32_t entityID) const;
	void	 EnsureReverseCapacity(uint32_t entityID);

	// Iterator over active packed data
	std::vector<T>							  &Data() { return m_data; }
	std::vector<uint32_t>					  &Index() { return m_index; }
	std::vector<uint32_t>					  &Reverse() { return m_reverse; }
	[[nodiscard]] const std::vector<uint32_t> &Index() const { return m_index; }
	[[nodiscard]] uint32_t					   GetCount() const override { return m_size; }
	void									   Clear() override;

private:
	std::vector<T>		  m_data	= std::vector<T>();			// packed component data
	std::vector<uint32_t> m_index	= std::vector<uint32_t>();	// maps packed-slot -> entityID
	std::vector<uint32_t> m_reverse = std::vector<uint32_t>();	// maps entityID -> packed index, UINT32_MAX if none

	SystemState m_state = SystemState::Uninitialized;
	uint32_t	m_size	= 0;
};

template <typename T>
ComponentArray<T>::~ComponentArray() {
	ComponentArray::Shutdown();
}

template <typename T>
ERROR_CODE ComponentArray<T>::Initialize(uint32_t maxCount) {
	PE_CHECK_STATE_INIT(m_state, "ComponentArray is already initialized.");
	m_state = SystemState::Initializing;
	m_data.reserve(maxCount);
	m_index.reserve(maxCount);
	m_reverse.assign(maxCount, UINT32_MAX);	 // pre-fill as unknown
	m_size = 0;

	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

template <typename T>
ERROR_CODE ComponentArray<T>::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;

	m_state = SystemState::ShuttingDown;
	m_data.clear();
	m_index.clear();
	m_reverse.clear();
	m_size = 0;

	m_state = SystemState::Uninitialized;
	return ERROR_CODE::OK;
}

template <typename T>
uint32_t ComponentArray<T>::Add(const uint32_t entityID, const void *componentData) {
	EnsureReverseCapacity(entityID);
	if (m_reverse[entityID] != UINT32_MAX) PE_LOG_FATAL("Entity already in reverse.");

	uint32_t packedIndex = m_size;
	m_data.push_back(*static_cast<const T *>(componentData));
	m_index.push_back(entityID);
	m_reverse[entityID] = packedIndex;
	m_size++;
	return entityID;
}

template <typename T>
RemovalInfo ComponentArray<T>::Remove(const uint32_t entityID) {
	if (entityID >= m_reverse.size()) PE_LOG_FATAL("Entity does not exist.");

	uint32_t packed = m_reverse[entityID];

	if (packed == UINT32_MAX || packed >= m_size) PE_LOG_FATAL("Entity does not exist.");

	uint32_t	   lastPacked = m_size - 1;
	const uint32_t lastEntity = m_index[lastPacked];

	// move last into 'packed' if not removing last
	if (packed != lastPacked) {
		m_data[packed]		  = std::move(m_data[lastPacked]);
		m_index[packed]		  = lastEntity;
		m_reverse[lastEntity] = packed;
	}

	m_data.pop_back();
	m_index.pop_back();

	m_reverse[entityID] = UINT32_MAX;
	m_size--;

	return {lastEntity, packed};  // lastEntity now at packed (or returned even if same)
}

template <typename T>
bool ComponentArray<T>::Has(uint32_t entityID) const {
	return entityID < m_reverse.size() && m_reverse[entityID] != UINT32_MAX && m_reverse[entityID] < m_size;
}

template <typename T>
T &ComponentArray<T>::Get(uint32_t entityID) {
	if (entityID >= m_reverse.size()) PE_LOG_FATAL("Entity ID can't bigger than reverse vector size!");

	uint32_t packed = m_reverse[entityID];
	assert(packed != UINT32_MAX);
	assert(packed < m_size);
	return m_data[packed];
}

template <typename T>
const T &ComponentArray<T>::Get(uint32_t entityID) const {
	assert(entityID < m_reverse.size());
	uint32_t packed = m_reverse[entityID];
	assert(packed != UINT32_MAX);
	assert(packed < m_size);
	return m_data[packed];
}

template <typename T>
void ComponentArray<T>::EnsureReverseCapacity(uint32_t entityID) {
	if (entityID >= m_reverse.size()) {
		size_t old = m_reverse.size();
		m_reverse.resize(entityID + 1, UINT32_MAX);
		PE_LOG_INFO("Resized reverse from " + std::to_string(old) + " to " + std::to_string(m_reverse.size()));
	}
}

template <typename T>
void ComponentArray<T>::Clear() {
	m_size = 0;
	std::fill(m_data.begin(), m_data.end(), T());
	std::fill(m_index.begin(), m_index.end(), UINT32_MAX);
	std::fill(m_reverse.begin(), m_reverse.end(), UINT32_MAX);

	m_data.clear();
	m_index.clear();
	m_reverse.clear();
}
}  // namespace PE::ECS