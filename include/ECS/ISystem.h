#pragma once

#include <atomic>
#include <cstdint>

#include "Common/Common.h"
#include "Utilities/Logger.h"

namespace PE::ECS {
enum class ESystemStage {
	EarlyUpdate = 0,
	Transform,
	Animation,
	Physics,
	SceneControl,
	GameLogic,	// TODO: For user scripts. Make an ordering mechanism between those logics.
	Camera,
	GUI,
	Particle,
	Render,
	LateUpdate,
	Count
};

using ISystemTypeID = uint32_t;
ISystemTypeID GenerateISystemTypeID();

class ISystem {
public:
	virtual ~ISystem()					  = default;
	virtual void	   OnUpdate(float dt) = 0;
	virtual ERROR_CODE Shutdown()		  = 0;

	[[nodiscard]] ISystemTypeID GetID() const;
	[[nodiscard]] ESystemStage	GetStage() const;
	template <typename TSystem>
	static ISystemTypeID GetUniqueISystemTypeID();

protected:
	ISystemTypeID m_typeID = UINT32_MAX;
	ESystemStage  m_stage  = ESystemStage::Count;
	SystemState	  m_state  = SystemState::Uninitialized;
};

inline ISystemTypeID GenerateISystemTypeID() {
	static std::atomic<uint32_t> lastID{0};
	return lastID.fetch_add(1, std::memory_order_relaxed);
}

inline ISystemTypeID ISystem::GetID() const {
	if (m_typeID != UINT32_MAX) return m_typeID;

	PE_LOG_FATAL("System type ID is invalid.");
	return UINT32_MAX;
}

inline ESystemStage ISystem::GetStage() const {
	if (m_stage != ESystemStage::Count) return m_stage;

	PE_LOG_FATAL("System stage is invalid");
	return ESystemStage::Count;
}

template <typename TISystem>
ISystemTypeID ISystem::GetUniqueISystemTypeID() {
	static_assert(std::is_base_of_v<ISystem, TISystem>, "TISystem must inherit from ISystem");
	static const uint32_t typeID = GenerateISystemTypeID();
	return typeID;
}
}  // namespace PE::ECS