#pragma once
#include <format>

#include "BroadPhaseCollisionSystem.h"
#include "Core/EngineConfig.h"
#include "ECS/ECSManager.h"
#include "Math/Math.h"
#include "Physics/Body/Components/BoxCollider.h"
#include "Physics/Body/Components/CapsuleCollider.h"
#include "Physics/Body/Components/CylinderCollider.h"
#include "Physics/Body/Components/KinematicBody.h"
#include "Physics/Body/Components/PhysicsMaterial.h"
#include "Physics/Body/Components/SphereCollider.h"
#include "Physics/Body/ContactResolver.h"
#include "Physics/Body/Types.h"
#include "PhysicsMaterialSystem.h"
#include "Scene/Components/Tag.h"
#include "Scene/Components/Transform.h"

namespace PE::Physics::Body::Systems {
class BodyPhysicsSystem;

class NarrowPhaseCollisionSystem {
public:
	explicit NarrowPhaseCollisionSystem()									  = default;
	NarrowPhaseCollisionSystem(const NarrowPhaseCollisionSystem &)			  = delete;
	NarrowPhaseCollisionSystem &operator=(const NarrowPhaseCollisionSystem &) = delete;
	NarrowPhaseCollisionSystem(NarrowPhaseCollisionSystem &&)				  = delete;
	NarrowPhaseCollisionSystem &operator=(NarrowPhaseCollisionSystem &&)	  = delete;
	~NarrowPhaseCollisionSystem()											  = default;

	void Initialize(ECS::ECSManager *ecsManager, const PE::Core::EngineConfig &config,
					PhysicsMaterialSystem *physicsMaterialSystem);
	void Shutdown();
	void OnUpdate(const CollisionPairs &collisionPairs, Math::real dt);
	void DetectCollisions(const CollisionPairs &collisionPairs);

	template <typename ContactType, typename Container, typename Func>
	void ProcessCollisions(const Container &pairs, std::vector<ContactType> &outList, Func &&collisionFunc,
						   bool flipNormal = false);

private:
	bool SphereAndSphere(ECS::EntityID idOne, ECS::EntityID idTwo, Math::RVec3 &outNormal, Math::RVec3 &outPoint,
						 Math::real &outPenetration) const;
	bool SphereAndCapsule(ECS::EntityID sphereID, ECS::EntityID capID, Math::RVec3 &outNormal, Math::RVec3 &outPoint,
						  Math::real &outPenetration) const;
	bool SphereAndCylinder(ECS::EntityID sphereID, ECS::EntityID cylID, Math::RVec3 &outNormal, Math::RVec3 &outPoint,
						   Math::real &outPenetration) const;
	bool CylinderAndCylinder(ECS::EntityID cylOneID, ECS::EntityID cylTwoID, Math::RVec3 &outNormal,
							 Math::RVec3 &outPoint, Math::real &outPenetration) const;
	[[nodiscard]] Math::RVec3 GetCylinderSupportPoint(const Math::RVec3 &center, const Math::RVec3 &upDir,
													  Math::real halfHeight, Math::real radius,
													  const Math::RVec3 &dir) const;
	bool CylinderAndBox(ECS::EntityID cylID, ECS::EntityID boxID, Math::RVec3 &outNormal, Math::RVec3 &outPoint,
						Math::real &outPenetration) const;
	bool CapsuleAndCylinder(ECS::EntityID capID, ECS::EntityID cylID, Math::RVec3 &outNormal, Math::RVec3 &outPoint,
							Math::real &outPenetration) const;
	bool CapsuleAndCapsule(ECS::EntityID capOneID, ECS::EntityID capTwoID, Math::RVec3 &outNormal,
						   Math::RVec3 &outPoint, Math::real &outPenetration) const;
	bool CapsuleAndBox(ECS::EntityID capID, ECS::EntityID boxID, Math::RVec3 &outNormal, Math::RVec3 &outPoint,
					   Math::real &outPenetration) const;
	bool BoxAndSphere(ECS::EntityID boxID, ECS::EntityID sphereID, Math::RVec3 &outNormal, Math::RVec3 &outPoint,
					  Math::real &outPenetration) const;
	bool BoxAndBox(ECS::EntityID entityOne, ECS::EntityID entityTwo, Math::RVec3 &outNormal, Math::RVec3 &outPoint,
				   Math::real &outPenetration);
	void FillPointFaceBoxBox(const Scene::Components::Transform &tfOne, const Scene::Components::Transform &tfTwo,
							 const Components::BoxCollider &colTwo, const Math::RVec3 &toCenter, uint32_t best,
							 Math::RVec3 &outNormal, Math::RVec3 &outPoint) const;
	Math::RVec3 ContactPoint(const Math::RVec3 &pOne, const Math::RVec3 &dOne, Math::real oneSize,
							 const Math::RVec3 &pTwo, const Math::RVec3 &dTwo, Math::real twoSize, bool useOne);
	Math::real	PenetrationOnAxis(const Components::BoxCollider &colOne, const Math::RQuat &orientOne,
								  const Components::BoxCollider &colTwo, const Math::RQuat &orientTwo,
								  const Math::RVec3 &axis, const Math::RVec3 &toCenter);
	Math::real TransformToAxis(const Math::RVec3 &halfExtents, const Math::RQuat &orientation, const Math::RVec3 &axis);
	template <typename SolidCol, typename ContainerCol>
	bool ResolveContainer(ECS::EntityID solidID, ECS::EntityID contID, Math::RVec3 &outNormal, Math::RVec3 &outPoint,
						  Math::real &outPen) const;
	static void ExtractPoints(const Components::SphereCollider &col, const Scene::Components::Transform &tf,
							  PointCloud &cloud);
	static void ExtractPoints(const Components::CapsuleCollider &col, const Scene::Components::Transform &tf,
							  PointCloud &cloud);
	static void ExtractPoints(const Components::BoxCollider &col, const Scene::Components::Transform &tf,
							  PointCloud &cloud);
	static void ExtractPoints(const Components::CylinderCollider &col, const Scene::Components::Transform &tf,
							  PointCloud &cloud);
	static bool TestPoint(const Components::SphereCollider &col, const Scene::Components::Transform &tf,
						  const Math::RVec3 &pt, Math::real r, Math::RVec3 &n, Math::RVec3 &outPt, Math::real &pen);
	static bool TestPoint(const Components::BoxCollider &col, const Scene::Components::Transform &tf,
						  const Math::RVec3 &pt, Math::real r, Math::RVec3 &n, Math::RVec3 &outPt, Math::real &pen);
	static bool TestPoint(const Components::CapsuleCollider &col, const Scene::Components::Transform &tf,
						  const Math::RVec3 &pt, Math::real r, Math::RVec3 &n, Math::RVec3 &outPt, Math::real &pen);
	static bool TestPoint(const Components::CylinderCollider &col, const Scene::Components::Transform &tf,
						  const Math::RVec3 &pt, Math::real r, Math::RVec3 &n, Math::RVec3 &outPt, Math::real &pen);

	ECS::ECSManager				 *ref_eM					= nullptr;
	PhysicsMaterialSystem		 *ref_physicsMaterialSystem = nullptr;
	std::vector<RigidBodyContact> m_rigidBodyContacts;
	std::vector<KinematicContact> m_kinematicContacts;
	std::vector<StaticContact>	  m_staticContacts;
};

template <typename ContactType, typename Container, typename Func>
void NarrowPhaseCollisionSystem::ProcessCollisions(const Container &pairs, std::vector<ContactType> &outList,
												   Func &&collisionFunc, const bool flipNormal) {
	for (const auto &[idOne, idTwo] : pairs) {
		if (ContactType contact{}; collisionFunc(idOne, idTwo, contact.normal, contact.point, contact.penetration)) {
			if (flipNormal) contact.normal *= -1.0f;

			contact.entityOne = idOne;

			// Only add entityTwo if it's a RigidBodyContact
			if constexpr (std::is_same_v<ContactType, RigidBodyContact>) contact.entityTwo = idTwo;

			// PRE-FETCH & BAKE: Kinematic bodies don't react to impulses, but their motion affects the contact.
			// We calculate and store the total surface velocity (Linear + Angular) at the contact point
			else if constexpr (std::is_same_v<ContactType, KinematicContact>) {
				if (ref_eM->HasComponent<Components::KinematicBody>(idTwo)) {
					const auto &kb					 = *ref_eM->GetTComponent<Components::KinematicBody>(idTwo);
					Math::RVec3 relativePos			 = contact.point - kb.position;
					contact.kinematicSurfaceVelocity = kb.velocity + Math::Cross(kb.angularVelocity, relativePos);
				} else {
					contact.kinematicSurfaceVelocity = Math::RVec3(0.0f);
				}
			}

			if (ref_eM->HasComponent<Components::PhysicsMaterial>(idOne) && ref_eM->HasComponent<
				    Components::PhysicsMaterial>(idTwo)) {
				auto *interactionPtr = ref_physicsMaterialSystem->GetMaterialInteraction(
					ref_eM->GetTComponent<Components::PhysicsMaterial>(idOne)->id,
					ref_eM->GetTComponent<Components::PhysicsMaterial>(idTwo)->id);

				if (interactionPtr) {
					MaterialInteraction const &interaction = *interactionPtr;
					contact.restitution                    = interaction.restitution;
					contact.staticFriction                 = interaction.staticFriction;
					contact.dynamicFriction                = interaction.dynamicFriction;
				}
			}

			outList.push_back(contact);
		}
	}
}

template <typename SolidCol, typename ContainerCol>
bool NarrowPhaseCollisionSystem::ResolveContainer(const ECS::EntityID solidID, const ECS::EntityID contID,
												  Math::RVec3 &outNormal, Math::RVec3 &outPoint,
												  Math::real &outPen) const {
	const auto &solidCol = *ref_eM->GetTComponent<SolidCol>(solidID);
	const auto &contCol	 = *ref_eM->GetTComponent<ContainerCol>(contID);
	const auto &solidTf	 = *ref_eM->GetTComponent<Scene::Components::Transform>(solidID);
	const auto &contTf	 = *ref_eM->GetTComponent<Scene::Components::Transform>(contID);

	PointCloud cloud;
	ExtractPoints(solidCol, solidTf, cloud);

	bool        collided = false;
	Math::real  maxPen   = -1.0f;
	Math::RVec3 bestNormal;
	Math::RVec3 bestPoint;

	for (size_t i = 0; i < cloud.count; ++i) {
		Math::RVec3 n;
		Math::RVec3 p;
		Math::real  pen;
		if (TestPoint(contCol, contTf, cloud.points[i], cloud.radii[i], n, p, pen)) {
			if (pen > maxPen) {
				maxPen	   = pen;
				bestNormal = n;
				bestPoint  = p;
				collided   = true;
			}
		}
	}

	if (collided) {
		outNormal = bestNormal;
		outPoint  = bestPoint;
		outPen	  = maxPen;
		return true;
	}
	return false;
}
}  // namespace PE::Physics::Body::Systems