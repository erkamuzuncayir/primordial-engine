#pragma once
#include <string>

namespace PE::Scene::Components {
struct Tag {
	// TODO: Convert this to a limited length type.
	std::string name = "Entity";
};
}  // namespace PE::Scene::Components