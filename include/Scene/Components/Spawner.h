#pragma once

#include <string>

#include "Math/Math.h"
#include "Physics/Body/Types.h"
#include "Physics/Body/Components/AABB.h"

namespace PE::Scene::Components {
struct UFrequency {
	bool isSingleBurst{false};

	union {
		struct {
			uint32_t count{1};
		} burst;

		struct {
			float	 timePassedLastSpawn{0};
			float	 interval{0};
			uint32_t maxCount{0};
		} repeating;
	};
};

struct ULocation {
	enum class Type : uint8_t { FixedLocation, RandomBox, RandomSphere };
	Type type{Type::FixedLocation};

	union {
		struct {
			Math::Vec3 position{0, 0, 0};
			Math::Vec3 orientationEuler{0, 0, 0};
			Math::Vec3 scale{1, 1, 1};
		} fixed;

		struct {
			Math::Vec3 min{0, 0, 0};
			Math::Vec3 max{1, 1, 1};
		} box;

		struct {
			Math::Vec3 center{0, 0, 0};
			float	   radius{1};
		} sphere;
	};
};

struct USizeRange {
	union {
		struct {
			Math::FloatRange radius{1, 1};
			Math::FloatRange height{1, 1};
		} capsuleAndCylinder;

		struct {
			Math::Vec3Range size{{1, 1, 1}, {1, 1, 1}};
		} cuboid;

		struct {
			Math::FloatRange radius{1, 1};
		} sphere;
	};
};

struct InitialVelocities {
	Math::Vec3Range linear{{0, 0, 0}, {0, 0, 0}};
	Math::Vec3Range angular{{0, 0, 0}, {0, 0, 0}};
};

enum class SpawnLocation { FixedLocation, RandomBox, RandomSphere };

struct Spawner {
	std::string								 name{};
	Physics::Body::Components::ColliderShape shape{};
	float									 startTime{0};
	UFrequency								 frequency{};
	ULocation								 location{};
	USizeRange								 sizeRange{};
	InitialVelocities						 velocities{};
	float									 density{1.0};
	Physics::Body::PhysicsMaterialID		 physicsMatID{Physics::Body::INVALID_PHYSICS_MATERIAL_ID};
	bool									 isGravityOn{true};
};
}  // namespace PE::Scene::Components