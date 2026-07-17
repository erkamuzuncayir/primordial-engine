#pragma once

#include <span>

#include "ECS/ECSManager.h"
#include "Physics/Body/Components/RigidBody.h"
#include "Types.h"

namespace PE::Physics::Body {

class ContactResolver {
public:
	static void ResolveContacts(ECS::ECSManager *eM, std::vector<StaticContact> &staticContacts,
								std::vector<KinematicContact> &kinematicContacts,
								std::vector<RigidBodyContact> &rigidBodyContacts, float dt);

private:
	template <typename SingleBodyContact>
	static void PrepareSingleBodyContacts(ECS::ECSManager *eM, std::vector<SingleBodyContact> &contacts, float dt);
	static void PrepareRigidBodyContacts(ECS::ECSManager *eM, std::vector<RigidBodyContact> &contacts, float dt);

	template <typename SingleBodyContact>
	static void CalculateContactInternals(SingleBodyContact &contact, const Components::RigidBody *rbOne,
										  Math::real dt);
	static void CalculateContactInternals(RigidBodyContact &contact, const Components::RigidBody *rbOne,
										  const Components::RigidBody *rbTwo, Math::real dt);

	static void						 CalculateContactBasis(Math::RMat33 &outContactToWorld, const Math::RVec3 &normal);
	[[nodiscard]] static Math::RVec3 CalculateLocalVelocity(const Components::RigidBody &rb,
															const Math::RVec3			&relativeContactPos,
															const Math::RMat33 &contactToWorld, Math::real dt);

	template <typename SingleBodyContact>
	static void CalculateDesiredDeltaVelocity(SingleBodyContact &contact, const Components::RigidBody *rbOne,
											  Math::real dt);
	static void CalculateDesiredDeltaVelocity(RigidBodyContact &contact, const Components::RigidBody *rbOne,
											  const Components::RigidBody *rbTwo, Math::real dt);

	static void AdjustPositions(ECS::ECSManager *eM, std::vector<StaticContact> &staticContacts,
								std::vector<KinematicContact> &kinematicContacts,
								std::vector<RigidBodyContact> &rigidBodyContacts);
	template <typename SingleBodyContact>
	static void ApplyPositionChange(Components::RigidBody &rb, const SingleBodyContact &contact,
									Math::RVec3 &positionChange, Math::RVec3 &orientationChange,
									const Math::real &penetration);

	static void ApplyPositionChange(Components::RigidBody &rbOne, Components::RigidBody &rbTwo,
									const RigidBodyContact &contact, Math::RVec3 positionChange[2],
									Math::RVec3 orientationChange[2], Math::real penetration);
	static void UpdatePositionsOfOtherContacts(std::vector<StaticContact>	 &staticContacts,
											   std::vector<KinematicContact> &kinematicContacts,
											   std::vector<RigidBodyContact> &rigidBodyContacts,
											   const ECS::EntityID moved[2], const Math::RVec3 linearChange[2],
											   const Math::RVec3 angularChange[2]);

	static void AdjustVelocities(ECS::ECSManager *eM, std::vector<StaticContact> &staticContacts,
								 std::vector<KinematicContact> &kinematicContacts,
								 std::vector<RigidBodyContact> &rigidBodyContacts, Math::real dt);
	template <typename SingleBodyContact>
	static void ApplyVelocityChange(Components::RigidBody &rb, const SingleBodyContact &contact,
									Math::RVec3 &velocityChange, Math::RVec3 &angularVelocityChange);
	static void ApplyVelocityChange(Components::RigidBody &rbOne, Components::RigidBody &rbTwo,
									const RigidBodyContact &contact, Math::RVec3 velocityChange[2],
									Math::RVec3 angularVelocityChange[2]);
	static void UpdateVelocitiesOfOtherContacts(ECS::ECSManager *eM, std::vector<StaticContact> &staticContacts,
												std::vector<KinematicContact> &kinematicContacts,
												std::vector<RigidBodyContact> &rigidBodyContacts,
												const ECS::EntityID moved[2], const Math::RVec3 velocityChange[2],
												const Math::RVec3 angularVelocityChange[2], Math::real dt);

	template <typename SingleBodyContact>
	static Math::RVec3 CalculateFrictionlessImpulse(const Components::RigidBody &rb, const SingleBodyContact &contact);
	static Math::RVec3 CalculateFrictionlessImpulse(const Components::RigidBody &rbOne,
													const Components::RigidBody &rbTwo,
													const RigidBodyContact		&contact);

	template <typename SingleBodyContact>
	static Math::RVec3 CalculateFrictionImpulse(const Components::RigidBody &rb, const SingleBodyContact &contact);
	static Math::RVec3 CalculateFrictionImpulse(const Components::RigidBody &rbOne, const Components::RigidBody &rbTwo,
												const RigidBodyContact &contact);
};
}  // namespace PE::Physics::Body