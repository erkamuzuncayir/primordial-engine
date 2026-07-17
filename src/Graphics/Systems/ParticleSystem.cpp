#include "Graphics/Systems/ParticleSystem.h"

#include <algorithm>

#include "Graphics/Components/ParticleEmitter.h"
#include "Graphics/IRenderer.h"
#include "Math/Random.h"
#include "Scene/Components/Transform.h"

namespace PE::Graphics::Systems {
ERROR_CODE ParticleSystem::Initialize(const ECS::ESystemStage stage, ECS::ECSManager *ecsManager, IRenderer *renderer) {
	PE_CHECK_STATE_INIT(m_state, "Particle system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID	 = GetUniqueISystemTypeID<ParticleSystem>();
	m_stage		 = stage;
	ref_eM		 = ecsManager;
	ref_renderer = renderer;
	m_state		 = SystemState::Running;
	return ERROR_CODE::OK;
}

ERROR_CODE ParticleSystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	m_state = SystemState::Uninitialized;
	return ERROR_CODE::OK;
}

void ParticleSystem::OnUpdate(float dt) {
	auto &compArr = ref_eM->GetCompArr<Graphics::Components::ParticleEmitter>();

	for (int i = 0; i < compArr.Data().size(); i++) {
		auto &emitter  = compArr.Data()[i];
		auto const &entityID = compArr.Index()[i];

		emitter.spawnAccumulator += dt;
		float rate = 1.0f / emitter.spawnRate;

		while (emitter.spawnAccumulator > rate) {
			if (emitter.particles.size() < emitter.maxParticles) {
				const auto *transform = ref_eM->TryGetTComponent<Scene::Components::Transform>(entityID);
				Math::Vec3	origin	  = transform ? transform->position : Math::Vec3(0.0f);

				Components::Particle p;
				p.position = origin;
				p.life	   = emitter.lifeTime;
				p.size	   = 1.0f;
				p.color	   = {1, 1, 1, 1};

				if (emitter.type == ParticleType::Fire) {
					float randomX = Math::Random::Get<float>(-1, 1) * (emitter.spawnRadius * 0.1f);
					float randomZ = Math::Random::Get<float>(-1, 1) * (emitter.spawnRadius * 0.1f);

					p.position.x += randomX;
					p.position.z += randomZ;

					p.velocity = {Math::Random::Get<float>(-1, 1) * 0.5f, 1.5f, Math::Random::Get<float>(-1, 1) * 0.5f};
					p.color	   = {1.0f, 0.9f, 0.6f, 1.0f};
				} else if (emitter.type == ParticleType::Rain || emitter.type == ParticleType::Snow) {
					p.position.y += 10.0f;

					p.position.x += Math::Random::Get<float>(-1, 1) * emitter.spawnRadius;
					p.position.z += Math::Random::Get<float>(-1, 1) * emitter.spawnRadius;

					if (emitter.type == ParticleType::Rain) {
						p.velocity = {0.0f, -15.0f, 0.0f};
						p.color	   = {0.8f, 0.8f, 1.0f, 0.6f};
					} else {
						p.velocity = {Math::Random::Get<float>(-1, 1) * 0.5f, -2.0f, Math::Random::Get<float>(-1, 1) * 0.5f};
						p.color	   = {1.0f, 1.0f, 1.0f, 0.9f};
					}
				} else if (emitter.type == ParticleType::Dust) {
					float angle = Math::Random::Get<float>(-1, 1) * Math::PI * 2.0f;

					float speed = 5.0f + (Math::Random::Get<float>(-1, 1) * 3.0f);

					p.velocity.x = cosf(angle) * speed;
					p.velocity.y = 0.1f + (Math::Random::Get<float>(-1, 1) * 0.2f);
					p.velocity.z = sinf(angle) * speed;

					p.position.x += (Math::Random::Get<float>(-1, 1) * 0.5f);
					p.position.z += (Math::Random::Get<float>(-1, 1) * 0.5f);

					p.color = {0.76f, 0.70f, 0.50f, 0.0f};

					p.size	   = 2.0f;
					p.rotation = Math::Random::Get<float>(-1, 1) * Math::PI;
				}
				emitter.particles.push_back(p);
			}
			emitter.spawnAccumulator -= rate;
		}

		for (int j = 0; j < emitter.particles.size(); ++j) {
			auto &p = emitter.particles[j];

			float lifeRatio = 1.0f - (p.life / emitter.lifeTime);

			p.life -= dt;
			p.position += p.velocity * dt;

			if (emitter.type == ParticleType::Fire) {
				float turbulence = sinf(p.position.y * 2.0f + p.life * 5.0f) * 1.5f;
				p.position.x += turbulence * dt;
				p.position.z += turbulence * dt;

				Math::Vec4 startColor = {1.0f, 0.9f, 0.6f, 1.0f};
				Math::Vec4 midColor	  = {1.0f, 0.4f, 0.0f, 0.9f};
				Math::Vec4 endColor	  = {0.1f, 0.1f, 0.1f, 0.0f};

				if (lifeRatio < 0.5f) {
					float t = lifeRatio / 0.5f;
					p.color = Math::Lerp(startColor, midColor, t);
					p.size	= Math::Lerp(1.0f, 0.8f, t);
				} else {
					float t = (lifeRatio - 0.5f) / 0.5f;
					p.color = Math::Lerp(midColor, endColor, t);
					p.size	= Math::Lerp(0.8f, 1.5f, t);
				}
			} else if (emitter.type == ParticleType::Dust) {
				float wave = sinf(p.life * 3.0f) * 2.0f;

				p.position.x += wave * dt;
				p.position.z += wave * dt;

				p.rotation += dt * 0.3f;

				p.size += dt * 3.0f;

				float maxAlpha = 0.3f;

				if (lifeRatio < 0.1f) {
					p.color.w = (lifeRatio / 0.1f) * maxAlpha;
				} else if (lifeRatio > 0.8f) {
					float remaining = (1.0f - lifeRatio) / 0.2f;
					p.color.w		= remaining * maxAlpha;
				} else {
					p.color.w = maxAlpha;
				}
			} else {
				p.color.w = std::clamp(p.life, 0.0f, 1.0f);
			}

			if (p.life <= 0.0f) {
				emitter.particles[j] = emitter.particles.back();
				emitter.particles.pop_back();
				j--;
			}
		}

		if (!emitter.particles.empty()) {
			ref_renderer->SubmitParticles(emitter.textureID, emitter.particles);
		}
	}
}
}  // namespace PE::Graphics::Systems