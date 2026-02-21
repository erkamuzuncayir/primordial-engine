#pragma once
#include <vector>

#include "Graphics/RenderTypes.h"
#include "Math/Math.h"

namespace PE::Graphics::Components {
struct Particle {
	Math::Vector3 position;
	Math::Vector3 velocity;
	Math::Vector4 color;
	float		  life;
	float		  size;
	float		  rotation;	 // Radians
};

struct ParticleEmitter {
	ParticleType type = ParticleType::Fire;

	uint32_t	  maxParticles = 2000;
	float		  spawnRate	   = 50.0f;	 // Particles per second
	float		  lifeTime	   = 2.0f;
	float		  spawnRadius  = 10.0f;
	Math::Vector3 velocityVar  = {0.5f, 1.0f, 0.5f};  // Random variance

	TextureID textureID = INVALID_HANDLE;

	// TODO: Change to fixed size general pool of array with max amount of particles.
	std::vector<Particle> particles;
	float				  spawnAccumulator = 0.0f;
};
}  // namespace PE::Graphics::Components