#pragma once
#include "Math/Math.h"

namespace PE::Physics::Body::Components {

enum class ColliderShape : uint32_t {
	Box,
	Sphere,
	Capsule,
	Cylinder,
	Count,
};

enum class ColliderType : uint32_t { Solid = 0, Container = 1 };

struct alignas(16) AABB {
	Math::RVec3 min;
	// Bits 0-27: Collision Layer (28 layers)
	// Bits 28-31: Shape Type (16 shapes)
	uint32_t collisionLayerAndShape;

	Math::RVec3 max;
	// Bit 31: Collider Type (0: Solid, 1: Container)
	// Bits 0-30: Collision Mask (31 bits for filtering)
	uint32_t collisionMask;

	[[nodiscard]] Math::real GetVolume() const { return (max.x - min.x) * (max.y - min.y) * (max.z - min.z); }

	void SetCollisionLayer(const uint32_t layer) {
		// Keep the top 4 bits (shape) intact, overwrite the bottom 28 bits
		collisionLayerAndShape = (collisionLayerAndShape & 0xF0000000) | (layer & 0x0FFFFFFF);
	}

	[[nodiscard]] uint32_t GetCollisionLayer() const {
		// Extract only the bottom 28 bits
		return collisionLayerAndShape & 0x0FFFFFFF;
	}

	void SetShapeType(const ColliderShape shape) {
		// Shift shape to the top 4 bits, keep the bottom 28 bits (layer) intact
		const uint32_t s	   = static_cast<uint32_t>(shape);
		collisionLayerAndShape = (collisionLayerAndShape & 0x0FFFFFFF) | (s << 28);
	}

	[[nodiscard]] ColliderShape GetShapeType() const {
		// Shift right by 28 to extract the top 4 bits
		return static_cast<ColliderShape>(collisionLayerAndShape >> 28);
	}

	void SetCollisionMask(const uint32_t mask) {
		// Keep the MSB (Collider Type) intact, overwrite the bottom 31 bits
		collisionMask = (collisionMask & 0x80000000) | (mask & 0x7FFFFFFF);
	}

	[[nodiscard]] uint32_t GetCollisionMask() const {
		// Extract only the bottom 31 bits
		return collisionMask & 0x7FFFFFFF;
	}

	void SetColliderType(const ColliderType type) {
		// Shift type to the 31st bit (MSB), keep the bottom 31 bits (mask) intact
		const uint32_t t = static_cast<uint32_t>(type);
		collisionMask	 = (collisionMask & 0x7FFFFFFF) | (t << 31);
	}

	[[nodiscard]] ColliderType GetColliderType() const {
		// Shift right by 31 to extract the Most Significant Bit
		return static_cast<ColliderType>(collisionMask >> 31);
	}

	[[nodiscard]] bool IsContainer() const { return (collisionMask >> 31) == static_cast<uint32_t>(ColliderType::Container); }

	[[nodiscard]] bool IsSolid() const { return (collisionMask >> 31) == static_cast<uint32_t>(ColliderType::Solid); }
};
}  // namespace PE::Physics::Body::Components