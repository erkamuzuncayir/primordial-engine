#include "Scene/Systems/DayNightSystem.h"

#include <algorithm>

#include "ECS/EntityManager.h"
#include "Graphics/Components/DirectionalLight.h"
#include "Graphics/Components/ParticleEmitter.h"
#include "Math/Math.h"
#include "Scene/Components/Tag.h"
#include "Scene/Components/Transform.h"

namespace PE::Scene::Systems {
ERROR_CODE DayNightSystem::Initialize(const ECS::ESystemStage stage, ECS::EntityManager *entityManager) {
	m_stage = stage;
	ref_eM	= entityManager;
	return ERROR_CODE::OK;
}

ERROR_CODE DayNightSystem::Shutdown() { return ERROR_CODE::OK; }

void DayNightSystem::OnUpdate(float dt) {
	auto &cycleArr = ref_eM->GetCompArr<Components::DayNightCycle>();

	for (int i = 0; i < cycleArr.Data().size(); ++i) {
		auto &cycle = cycleArr.Data()[i];

		float timeStep = (24.0f / cycle.dayDuration) * dt;
		cycle.timeOfDay += timeStep;
		if (cycle.timeOfDay >= 24.0f) cycle.timeOfDay -= 24.0f;

		cycle.seasonTimer += dt;
		if (cycle.seasonTimer >= cycle.seasonDuration) {
			cycle.seasonTimer = 0.0f;

			int nextSeason		= (static_cast<int>(cycle.currentSeason) + 1) % 4;
			cycle.currentSeason = static_cast<Components::Season>(nextSeason);
		}

		UpdateSunMoon(cycle, dt);
		UpdateWeather(cycle, dt);

		UpdateEnvironmentalEffects(cycle, dt);
	}
}

void DayNightSystem::IncreaseCycleDayDuration() const {
	auto &cycleArr = ref_eM->GetCompArr<Components::DayNightCycle>();
	for (int i = 0; i < cycleArr.Data().size(); ++i) {
		auto &cycle = cycleArr.Data()[i];
		if (cycle.dayDuration < MaxDayDuration) cycle.dayDuration++;
	}
}

void DayNightSystem::DecreaseCycleDayDuration() const {
	auto &cycleArr = ref_eM->GetCompArr<Components::DayNightCycle>();
	for (int i = 0; i < cycleArr.Data().size(); ++i) {
		auto &cycle = cycleArr.Data()[i];
		if (cycle.dayDuration > MinDayDuration) cycle.dayDuration--;
	}
}

void DayNightSystem::UpdateSunMoon(Components::DayNightCycle &cycle, float dt) {
	float	   angleRad = ((cycle.timeOfDay - 6.0f) / 24.0f) * Math::PI * 2.0f;
	float	   cosA		= cos(angleRad);
	float	   sinA		= sin(angleRad);
	Math::Vec3 sunDir	= {cosA, sinA, 0.2f};
	Math::Vec3 moonDir	= {-cosA, -sinA, 0.2f};

	sunDir	= Math::Normalize(sunDir);
	moonDir = Math::Normalize(moonDir);

	const float SWITCH_THRESHOLD = -0.2f;

	const float FADE_RANGE = 0.15f;

	if (cycle.sunEntity != ECS::INVALID_ENTITY_ID) {
		auto *sunTrans = ref_eM->TryGetTIComponent<Components::Transform>(cycle.sunEntity);
		auto *sunLight = ref_eM->TryGetTIComponent<Graphics::Components::DirectionalLight>(cycle.sunEntity);

		if (sunTrans) {
			sunTrans->position = sunDir * 105.0f;
			sunTrans->state	   = Components::Transform::TransformState::Dirty;
		}
		if (sunLight) {
			Math::Vec4 baseColor = CalculateLightColor(sunDir.y, cycle);

			float fade = std::clamp((sunDir.y - SWITCH_THRESHOLD) / FADE_RANGE, 0.0f, 1.0f);

			float dayIntensity = 1.5f;

			sunLight->color = Math::Vec4(baseColor.x, baseColor.y, baseColor.z, fade * dayIntensity);
		}
	}

	if (cycle.moonEntity != ECS::INVALID_ENTITY_ID) {
		auto *moonTrans = ref_eM->TryGetTIComponent<Components::Transform>(cycle.moonEntity);
		auto *moonLight = ref_eM->TryGetTIComponent<Graphics::Components::DirectionalLight>(cycle.moonEntity);

		if (moonTrans) {
			moonTrans->position = moonDir * 105.0f;
			moonTrans->state	= Components::Transform::TransformState::Dirty;
		}
		if (moonLight) {
			float fade = std::clamp((SWITCH_THRESHOLD - sunDir.y) / FADE_RANGE, 0.0f, 1.0f);

			float moonIntensity = 0.5f;

			moonLight->color =
				Math::Vec4(cycle.moonColor.x, cycle.moonColor.y, cycle.moonColor.z, fade * moonIntensity);
		}
	}

	if (sunDir.y > SWITCH_THRESHOLD) {
		cycle.activeLightEntity = cycle.sunEntity;
	} else {
		cycle.activeLightEntity = cycle.moonEntity;
	}
}

void DayNightSystem::UpdateWeather(Components::DayNightCycle &cycle, float dt) {
	if (cycle.weatherEntity == ECS::INVALID_ENTITY_ID) return;

	auto *emitter = ref_eM->TryGetTIComponent<Graphics::Components::ParticleEmitter>(cycle.weatherEntity);
	if (!emitter) return;

	switch (cycle.currentSeason) {
		case Components::Season::Winter:
			if (emitter->type != Graphics::ParticleType::Snow) {
				emitter->type	   = Graphics::ParticleType::Snow;
				emitter->textureID = cycle.snowTexture;
				emitter->particles.clear();
				emitter->spawnRate = 1000.0f;
			}
			break;

		case Components::Season::Autumn:
		case Components::Season::Spring:
			if (emitter->type != Graphics::ParticleType::Rain) {
				emitter->type	   = Graphics::ParticleType::Rain;
				emitter->textureID = cycle.rainTexture;
				emitter->particles.clear();
				emitter->spawnRate = 3000.0f;
			}
			break;

		case Components::Season::Summer: emitter->spawnRate = 0.0f; break;
	}
}

void DayNightSystem::UpdateEnvironmentalEffects(Components::DayNightCycle &cycle, float dt) {
	using namespace Graphics::Components;

	float angleRad = ((cycle.timeOfDay - 6.0f) / 24.0f) * Math::PI * 2.0f;
	float sunY	   = sin(angleRad);

	bool isDay			 = (sunY > -0.2f);
	bool isPrecipitating = (cycle.currentSeason != Components::Season::Summer);

	auto &tagArr = ref_eM->GetCompArr<Components::Tag>();

	float growthSpeed = 0.05f * dt;

	for (int j = 0; j < tagArr.GetCount(); ++j) {
		const std::string &name = tagArr.Data()[j].name;

		if (name.find("Tree") != std::string::npos || name.find("Saguaro") != std::string::npos ||
			name.find("Joshua") != std::string::npos || name.find("Candelabra") != std::string::npos) {
			ECS::EntityID treeID	= tagArr.Index()[j];
			auto		 *transform = ref_eM->TryGetTIComponent<Components::Transform>(treeID);

			if (transform) {
				float currentScale = transform->scale.x;

				if (isPrecipitating) {
					if (currentScale < 1.5f) {
						currentScale += growthSpeed;
					}
				} else if (isDay) {
					if (currentScale > 0.5f) {
						currentScale -= growthSpeed * 0.5f;
					}
				}

				transform->scale = Math::Vec3(currentScale);
				transform->state = Components::Transform::TransformState::Dirty;
			}
		}
	}

	if (cycle.dustEntity != ECS::INVALID_ENTITY_ID) {
		if (auto *dust = ref_eM->TryGetTIComponent<ParticleEmitter>(cycle.dustEntity)) {
			bool active		= (!isDay && !isPrecipitating);
			dust->spawnRate = active ? 20.0f : 0.0f;
		}
	}

	if (cycle.bonfireEntity != ECS::INVALID_ENTITY_ID) {
		if (auto *fire = ref_eM->TryGetTIComponent<ParticleEmitter>(cycle.bonfireEntity)) {
			bool active		= (isDay && !isPrecipitating);
			fire->spawnRate = active ? 50.0f : 0.0f;
		}
	}
}

Math::Vec4 DayNightSystem::CalculateLightColor(float sunHeight, const Components::DayNightCycle &cycle) {
	if (sunHeight < 0.0f) return cycle.nightColor;
	if (sunHeight > 0.2f) return cycle.dayColor;

	float t = std::clamp(sunHeight / 0.2f, 0.0f, 1.0f);

	t = t * t * (3.0f - 2.0f * t);

	return Math::Lerp(cycle.dawnColor, cycle.dayColor, t);
}
}  // namespace PE::Scene::Systems