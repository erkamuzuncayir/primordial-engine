#pragma once
#include "Assets/AssetManager.h"
#include "ECS/EntityManager.h"

namespace PE::Scene {
class EntityFactory {
public:
	EntityFactory() = delete;
	static ERROR_CODE				   Initialize(ECS::EntityManager *entityManager);
	static void						   Shutdown();
	[[nodiscard]] static ECS::EntityID CreatePrimitive(Graphics::PrimitiveType type);

private:
	static inline ECS::EntityManager *ref_eM		  = nullptr;
	static inline bool				  m_isInitialized = false;
};
}  // namespace PE::Scene