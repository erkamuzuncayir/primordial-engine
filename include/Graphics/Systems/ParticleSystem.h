#pragma once
#include "Common/Common.h"
#include "ECS/EntityManager.h"
#include "ECS/ISystem.h"
#include "Graphics/IRenderer.h"

namespace PE::Graphics::Systems {
class ParticleSystem : public ECS::ISystem {
public:
	ParticleSystem()		   = default;
	~ParticleSystem() override = default;

	ERROR_CODE Initialize(ECS::ESystemStage stage, ECS::EntityManager *entityManager, IRenderer *renderer);
	ERROR_CODE Shutdown() override;
	void	   OnUpdate(float dt) override;

private:
	ECS::EntityManager *ref_eM		 = nullptr;
	IRenderer		   *ref_renderer = nullptr;
};
}  // namespace PE::Graphics::Systems