#pragma once
#include "Math/Math.h"

namespace PE::Scene::Components {
struct Transform {
	enum class TransformState : uint8_t { Clean, Dirty, Updated };

	Math::Vec3 position{0.0f, 0.0f, 0.0f};
	Math::Vec3 rotation{0.0f, 0.0f, 0.0f};
	Math::Vec3 scale{1.0f, 1.0f, 1.0f};

	Math::Mat44 localMatrix{Math::Mat44Identity()};
	Math::Mat44 worldMatrix{Math::Mat44Identity()};

	uint32_t parentEntityID	   = UINT32_MAX;
	uint32_t parentPackedIndex = UINT32_MAX;

	TransformState state = TransformState::Clean;
};
}  // namespace PE::Scene::Components