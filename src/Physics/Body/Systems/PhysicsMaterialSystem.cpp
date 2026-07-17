#include "Physics/Body/Systems/PhysicsMaterialSystem.h"

#include <format>

namespace PE::Physics::Body::Systems {

void PhysicsMaterialSystem::Reset() {
	m_matInteractions.clear();
	m_physicsMaterialNamesAndDensity.clear();
}

PhysicsMaterialID PhysicsMaterialSystem::AddPhysicsMaterial(const std::string &name, const Math::real density) {
	m_physicsMaterialNamesAndDensity.emplace_back(name, density);
	return m_physicsMaterialNamesAndDensity.size() - 1;
}

PhysicsMaterialID PhysicsMaterialSystem::GetPhysicsMaterialID(const std::string &name) const {
	for (int i = 0; i < m_physicsMaterialNamesAndDensity.size(); i++) {
		if (m_physicsMaterialNamesAndDensity[i].first == name) return i;
	}
	PE_LOG_WARN(std::format("Physics Material Name is not found: {}", name));
	return INVALID_PHYSICS_MATERIAL_ID;
}

std::string *PhysicsMaterialSystem::GetPhysicsMaterialName(const PhysicsMaterialID id) {
	if (m_physicsMaterialNamesAndDensity.size() > id) {
		return &m_physicsMaterialNamesAndDensity[id].first;
	}
	PE_LOG_WARN(std::format("ID not found: {}!", id));
	return nullptr;
}

Math::real PhysicsMaterialSystem::GetPhysicsMaterialDensity(PhysicsMaterialID id) const {
	if (m_physicsMaterialNamesAndDensity.size() > id) {
		return m_physicsMaterialNamesAndDensity[id].second;
	}
	PE_LOG_WARN(std::format("ID not found: {}!", id));
	return 0;
}

Math::real PhysicsMaterialSystem::GetPhysicsMaterialDensity(const std::string_view name) const {
	for (int i = 0; i < m_physicsMaterialNamesAndDensity.size(); i++) {
		if (m_physicsMaterialNamesAndDensity[i].first == name) return m_physicsMaterialNamesAndDensity[i].second;
	}
	return 0;
}

void PhysicsMaterialSystem::AddMaterialInteraction(const PhysicsMaterialID a, const PhysicsMaterialID b,
												   const MaterialInteraction interaction) {
	m_matInteractions[GetMaterialInteractionKey(a, b)] = interaction;
}

MaterialInteraction *PhysicsMaterialSystem::GetMaterialInteraction(const PhysicsMaterialID matA,
																   const PhysicsMaterialID matB) {
	if (const auto it = m_matInteractions.find(GetMaterialInteractionKey(matA, matB)); it != m_matInteractions.end()) {
		return &(it->second);
	}

	return nullptr;
}

uint64_t PhysicsMaterialSystem::GetMaterialInteractionKey(const PhysicsMaterialID a, const PhysicsMaterialID b) const {
	return (static_cast<uint64_t>(std::min(a, b)) << 32) | std::max(a, b);
}
}  // namespace PE::Physics::Body::Systems