#pragma once
#include "ECS/Entity.h"
#include "Graphics/RenderTypes.h"
#include "Math/Math.h"

namespace PE::Scene::Components {
enum class Season { Spring, Summer, Autumn, Winter };

struct DayNightCycle {
	float timeOfDay		 = 8.0f;   // 0.0 to 24.0
	float dayDuration	 = 60.0f;  // Seconds for a full 24h cycle
	float seasonTimer	 = 0.0f;
	float seasonDuration = 300.0f;	// Seconds per season

	Season currentSeason = Season::Spring;

	ECS::EntityID sunEntity		= ECS::INVALID_ENTITY_ID;
	ECS::EntityID moonEntity	= ECS::INVALID_ENTITY_ID;
	ECS::EntityID weatherEntity = ECS::INVALID_ENTITY_ID;
	ECS::EntityID dustEntity	= ECS::INVALID_ENTITY_ID;
	ECS::EntityID bonfireEntity = ECS::INVALID_ENTITY_ID;

	ECS::EntityID activeLightEntity = ECS::INVALID_ENTITY_ID;

	Graphics::TextureID rainTexture = Graphics::INVALID_HANDLE;
	Graphics::TextureID snowTexture = Graphics::INVALID_HANDLE;

	Math::Vector4 dayColor	 = {1.0f, 0.95f, 0.8f, 1.0f};
	Math::Vector4 dawnColor	 = {1.0f, 0.4f, 0.2f, 1.0f};
	Math::Vector4 nightColor = {0.05f, 0.05f, 0.1f, 1.0f};
	Math::Vector4 moonColor	 = {0.6f, 0.7f, 0.9f, 0.5f};
};
}  // namespace PE::Scene::Components