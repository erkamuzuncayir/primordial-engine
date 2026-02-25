#pragma once
#include <vector>

#include "Graphics/RenderTypes.h"
#include "Math/Math.h"

namespace PE::Graphics::Components {
struct Particle {
	Math::Vec3 position;
	Math::Vec3 velocity;
	Math::Vec4 color;
	float	   life;
	float	   size;
	float	   rotation;  // Radians
};

struct ParticleEmitter {
	ParticleType type = ParticleType::Fire;

	uint32_t   maxParticles = 2000;
	float	   spawnRate	= 50.0f;  // Particles per second
	float	   lifeTime		= 2.0f;
	float	   spawnRadius	= 10.0f;
	Math::Vec3 velocityVar	= {0.5f, 1.0f, 0.5f};  // Random variance

	TextureID textureID = INVALID_HANDLE;

	// TODO: Change to fixed size general pool of array with max amount of particles.
	std::vector<Particle> particles;
	float				  spawnAccumulator = 0.0f;
};
}  // namespace PE::Graphics::Components