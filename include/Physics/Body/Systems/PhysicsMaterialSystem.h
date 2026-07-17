#pragma once
#include <unordered_map>

#include "ECS/ECSManager.h"
#include "Physics/Body/Types.h"

namespace PE::Physics::Body::Systems {

class PhysicsMaterialSystem {
public:
	explicit PhysicsMaterialSystem()								= default;
	PhysicsMaterialSystem(const PhysicsMaterialSystem &)			= delete;
	PhysicsMaterialSystem &operator=(const PhysicsMaterialSystem &) = delete;
	PhysicsMaterialSystem(PhysicsMaterialSystem &&)					= delete;
	PhysicsMaterialSystem &operator=(PhysicsMaterialSystem &&)		= delete;
	~PhysicsMaterialSystem()										= default;

	void			  Reset();
	PhysicsMaterialID AddPhysicsMaterial(const std::string &name, Math::real density);
	PhysicsMaterialID GetPhysicsMaterialID(const std::string &name) const;
	std::string		 *GetPhysicsMaterialName(PhysicsMaterialID id);
	Math::real		  GetPhysicsMaterialDensity(PhysicsMaterialID id) const;
	Math::real		  GetPhysicsMaterialDensity(std::string_view name) const;
	void			  AddMaterialInteraction(PhysicsMaterialID a, PhysicsMaterialID b, MaterialInteraction interaction);

	MaterialInteraction *GetMaterialInteraction(PhysicsMaterialID matA, PhysicsMaterialID matB);

private:
	uint64_t GetMaterialInteractionKey(PhysicsMaterialID a, PhysicsMaterialID b) const;
	std::vector<std::pair<std::string, Math::real>>	  m_physicsMaterialNamesAndDensity;
	std::unordered_map<uint64_t, MaterialInteraction> m_matInteractions;
};
}  // namespace PE::Physics::Body::Systems