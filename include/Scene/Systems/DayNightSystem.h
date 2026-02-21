#pragma once
#include "ECS/EntityManager.h"
#include "ECS/ISystem.h"
#include "Scene/Components/DayNightCycle.h"

namespace PE::Scene::Systems {
class DayNightSystem : public ECS::ISystem {
public:
	DayNightSystem()		   = default;
	~DayNightSystem() override = default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::EntityManager *entityManager);
	ERROR_CODE Shutdown() override;
	void	   OnUpdate(float dt) override;
	void	   IncreaseCycleDayDuration() const;
	void	   DecreaseCycleDayDuration() const;

private:
	void		  UpdateSunMoon(Components::DayNightCycle &cycle, float dt);
	void		  UpdateWeather(Components::DayNightCycle &cycle, float dt);
	void		  UpdateEnvironmentalEffects(Components::DayNightCycle &cycle, float dt);
	Math::Vector4 CalculateLightColor(float sunHeight, const Components::DayNightCycle &cycle);

	ECS::EntityManager *ref_eM = nullptr;

	static constexpr float MaxDayDuration = 60.0f;
	static constexpr float MinDayDuration = 1.0f;
};
}  // namespace PE::Scene::Systems