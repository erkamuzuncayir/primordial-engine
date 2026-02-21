#pragma once
#include "Math/Math.h"

namespace PE::Scene::Components {
struct Transform {
	enum class TransformState : uint8_t { Clean, Dirty, Updated };

	Math::Vector3 position{0.0f, 0.0f, 0.0f};
	Math::Vector3 rotation{0.0f, 0.0f, 0.0f};
	Math::Vector3 scale{1.0f, 1.0f, 1.0f};

	Math::Matrix4 localMatrix{Math::Matrix4Identity()};
	Math::Matrix4 worldMatrix{Math::Matrix4Identity()};

	uint32_t parentEntityID	   = UINT32_MAX;
	uint32_t parentPackedIndex = UINT32_MAX;

	TransformState state = TransformState::Clean;
};
}  // namespace PE::Scene::Components