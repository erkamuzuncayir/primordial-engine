#include "Graphics/Systems/GUISystem.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <format>

#include "Assets/AssetManager.h"
#include "ECS/ECSManager.h"
#include "Graphics/Components/Camera.h"
#include "Graphics/Components/DirectionalLight.h"
#include "Graphics/Components/MeshRenderer.h"
#include "Graphics/RenderTypes.h"
#include "Physics/Body/Components/Aero.h"
#include "Physics/Body/Components/AeroControl.h"
#include "Physics/Body/Components/AnchoredBungee.h"
#include "Physics/Body/Components/AnchoredSpring.h"
#include "Physics/Body/Components/AngledAero.h"
#include "Physics/Body/Components/BoxCollider.h"
#include "Physics/Body/Components/Buoyancy.h"
#include "Physics/Body/Components/CapsuleCollider.h"
#include "Physics/Body/Components/CylinderCollider.h"
#include "Physics/Body/Components/Drag.h"
#include "Physics/Body/Components/SphereCollider.h"
#include "Physics/Body/Components/Spring.h"
#include "Physics/Body/Components/StaticEntity.h"
#include "Physics/Core/Systems/PhysicsSystem.h"
#include "Physics/Particle/Components/AnchoredBungee.h"
#include "Physics/Particle/Components/AnchoredSpring.h"
#include "Physics/Particle/Components/Buoyancy.h"
#include "Physics/Particle/Components/CableConstraint.h"
#include "Physics/Particle/Components/Drag.h"
#include "Physics/Particle/Components/PointMass.h"
#include "Physics/Particle/Components/RodConstraint.h"
#include "Physics/Particle/Components/Spring.h"
#include "Scene/Components/DayNightCycle.h"
#include "Scene/Components/Tag.h"
#include "Scene/Components/WaypointAnimation.h"
#include "Scene/Systems/SceneManager.h"
#include "Utilities/EnumReflection.h"
#include "Utilities/IOUtilities.h"
#include "Utilities/Logger.h"

namespace PE::Physics::Components {
struct PointMass;
}

namespace PE::Scene::Components {
enum class Season;
struct DayNightCycle;
}  // namespace PE::Scene::Components

namespace PE::Graphics::Components {
struct DirectionalLight;
}

namespace PE::Graphics::Systems {
ERROR_CODE GUISystem::Initialize(const ECS::ESystemStage stage, ECS::ECSManager *ecsManager,
								 Physics::Core::Systems::PhysicsSystem *physicsSystem, Scene::Systems::SceneManager* sceneManager, Scene::SceneLoader *sceneLoader,
								 IRenderer *renderer) {
	PE_CHECK_STATE_INIT(m_state, "GUI system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID		  = GetUniqueISystemTypeID<GUISystem>();
	m_stage			  = stage;
	ref_eM			  = ecsManager;
	ref_physicsSystem = physicsSystem;
	ref_renderer	  = renderer;
	ref_sceneLoader	  = sceneLoader;
	ref_sceneManager = sceneManager;

	m_fpsTimer.Reset();
	PE_LOG_INFO("GUI System Initialized.");
	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

ERROR_CODE GUISystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;
	m_state = SystemState::Uninitialized;
	return ERROR_CODE::OK;
}

void GUISystem::OnUpdate(const float dt) {
	if (m_state != SystemState::Running) return;


	if (m_shouldRender) {
		DrawPerformanceStats(dt);
		DrawTopBar();
		DrawHierarchy();
		DrawInspector();
		DrawAssetBrowser();
	}

	Render();
}

void GUISystem::ToggleGUI() { m_shouldRender = !m_shouldRender; }

void GUISystem::Render() const { ImGui::Render(); }

void GUISystem::DrawHierarchy() {
	if (!ImGui::Begin("Hierarchy")) {
		ImGui::End();
		return;
	}

	if (ImGui::Button("Add Entity", ImVec2(-1, 0))) {
		ECS::EntityID newEntity = ref_eM->CreateEntity();
		ref_eM->AddComponent(newEntity, Scene::Components::Tag{"New Entity"});
		m_selectedEntity = newEntity;
	}
	ImGui::Separator();

	std::unordered_map<uint32_t, std::vector<uint32_t>> childMap;
	std::vector<uint32_t>								rootNodes;

	auto	   &transformArr = ref_eM->GetCompArr<PE::Scene::Components::Transform>();
	const auto &indices		 = transformArr.Index();
	const auto &data		 = transformArr.Data();
	size_t		count		 = transformArr.GetCount();

	for (size_t i = 0; i < count; ++i) {
		uint32_t	entityID = indices[i];
		const auto &tf		 = data[i];

		if (tf.parentEntityID == ECS::INVALID_ENTITY_ID) {
			rootNodes.push_back(entityID);
		} else {
			childMap[tf.parentEntityID].push_back(entityID);
		}
	}

	for (uint32_t rootID : rootNodes) {
		DrawEntityNodeRecursive(rootID, childMap);
	}

	if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
		m_selectedEntity = ECS::INVALID_ENTITY_ID;
	}

	ImGui::End();
}

void GUISystem::DrawEntityNodeRecursive(uint32_t												   entityID,
										const std::unordered_map<uint32_t, std::vector<uint32_t>> &childrenMap) {
	std::string name = std::format("Entity {}", entityID);
	if (const auto *tag = ref_eM->TryGetTComponent<PE::Scene::Components::Tag>(entityID)) {
		if (!tag->name.empty()) name = tag->name;
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (m_selectedEntity == entityID) flags |= ImGuiTreeNodeFlags_Selected;

	const bool hasChildren = childrenMap.contains(entityID);
	if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	const bool opened = ImGui::TreeNodeEx(reinterpret_cast<void *>(static_cast<uint64_t>(entityID)), flags, "%s", name.c_str());

	if (ImGui::IsItemClicked()) {
		m_selectedEntity = entityID;
	}

	if (opened && hasChildren) {
		for (const uint32_t childID : childrenMap.at(entityID)) {
			DrawEntityNodeRecursive(childID, childrenMap);
		}
		ImGui::TreePop();
	}
}

void GUISystem::DrawInspector() const {
	if (!ImGui::Begin("Inspector")) {
		ImGui::End();
		return;
	}

	if (m_selectedEntity != ECS::INVALID_ENTITY_ID) {
		DrawComponents();

		if (ImGui::Button("Add component")) {
			ImGui::OpenPopup("AddComponent");
		}
		if (ImGui::BeginPopup("AddComponent")) {
			ImGui::SeparatorText("Components");

			auto addComp = [this]<typename T>(const char *name, T * = nullptr) {
				if (ImGui::Selectable(name)) {
					if (!ref_eM->HasComponent<T>(m_selectedEntity)) {
						ref_eM->AddComponent(m_selectedEntity, T());
					}
				}
			};

			addComp("Tag", static_cast<Scene::Components::Tag *>(nullptr));
			addComp("Transform", static_cast<Scene::Components::Transform *>(nullptr));
			addComp("Camera", static_cast<Components::Camera *>(nullptr));
			addComp("DirectionalLight", static_cast<Components::DirectionalLight *>(nullptr));
			addComp("MeshRenderer", static_cast<Components::MeshRenderer *>(nullptr));
			addComp("ParticleEmitter", static_cast<Components::ParticleEmitter *>(nullptr));
			addComp("DayNightCycle", static_cast<Scene::Components::DayNightCycle *>(nullptr));
			addComp("AABB", static_cast<Physics::Body::Components::AABB *>(nullptr));
			addComp("RigidBody", static_cast<Physics::Body::Components::RigidBody *>(nullptr));
			addComp("KinematicBody", static_cast<Physics::Body::Components::KinematicBody *>(nullptr));
			addComp("StaticEntity", static_cast<Physics::Body::Components::StaticEntity *>(nullptr));
			addComp("BoxCollider", static_cast<Physics::Body::Components::BoxCollider *>(nullptr));
			addComp("SphereCollider", static_cast<Physics::Body::Components::SphereCollider *>(nullptr));
			addComp("CapsuleCollider", static_cast<Physics::Body::Components::CapsuleCollider *>(nullptr));
			addComp("CylinderCollider", static_cast<Physics::Body::Components::CylinderCollider *>(nullptr));
			addComp("Aero", static_cast<Physics::Body::Components::Aero *>(nullptr));
			addComp("AeroControl", static_cast<Physics::Body::Components::AeroControl *>(nullptr));
			addComp("AngledAero", static_cast<Physics::Body::Components::AngledAero *>(nullptr));
			addComp("BodyDrag", static_cast<Physics::Body::Components::Drag *>(nullptr));
			addComp("BodyBuoyancy", static_cast<Physics::Body::Components::Buoyancy *>(nullptr));
			addComp("BodySpring", static_cast<Physics::Body::Components::Spring *>(nullptr));
			addComp("BodyAnchoredSpring", static_cast<Physics::Body::Components::AnchoredSpring *>(nullptr));
			addComp("BodyAnchoredBungee", static_cast<Physics::Body::Components::AnchoredBungee *>(nullptr));
			addComp("PhysicsMaterial", static_cast<Physics::Body::Components::PhysicsMaterial *>(nullptr));
			addComp("PointMass", static_cast<Physics::Particle::Components::PointMass *>(nullptr));
			addComp("ParticleDrag", static_cast<Physics::Particle::Components::Drag *>(nullptr));
			addComp("ParticleBuoyancy", static_cast<Physics::Particle::Components::Buoyancy *>(nullptr));
			addComp("ParticleSpring", static_cast<Physics::Particle::Components::Spring *>(nullptr));
			addComp("ParticleAnchoredSpring", static_cast<Physics::Particle::Components::AnchoredSpring *>(nullptr));
			addComp("ParticleAnchoredBungee", static_cast<Physics::Particle::Components::AnchoredBungee *>(nullptr));
			addComp("CableConstraint", static_cast<Physics::Particle::Components::CableConstraint *>(nullptr));
			addComp("RodConstraint", static_cast<Physics::Particle::Components::RodConstraint *>(nullptr));
			addComp("Spawner", static_cast<Scene::Components::Spawner *>(nullptr));
			addComp("WaypointAnimation", static_cast<Scene::Components::WaypointAnimation *>(nullptr));

			ImGui::EndPopup();
		}
	} else {
		ImGui::Text("Select an entity from Hierarchy.");
	}

	ImGui::End();
}

void GUISystem::DrawComponents() const {
	// Queue removals/resets to run after all UI interactions are safely finished
	std::vector<std::function<void()>> deferredActions;

	auto deferCompMenu = [this, &deferredActions]<typename T>(T*) {
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Reset Values")) {
				deferredActions.push_back([this]() {
					ref_eM->RemoveComponent<T>(m_selectedEntity);
					ref_eM->AddComponent(m_selectedEntity, T());
				});
			}
			if (ImGui::MenuItem("Remove Component")) {
				deferredActions.push_back([this]() {
					ref_eM->RemoveComponent<T>(m_selectedEntity);
				});
			}
			ImGui::EndPopup();
		}
	};

	auto deferCompRemoveOnly = [this, &deferredActions]<typename T>(T*) {
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::MenuItem("Remove Component")) {
				deferredActions.push_back([this]() {
					ref_eM->RemoveComponent<T>(m_selectedEntity);
				});
			}
			ImGui::EndPopup();
		}
	};

	auto makeHeader = [this]<typename T>(const char *name, T *comp) {
		static std::string buf;
		size_t idx = comp - ref_eM->GetCompArr<T>().Data().data();
		buf = std::format("{} (Idx: {})###{}", name, idx, name);
		return buf.c_str();
	};

	// Note: To render common properties for spawner sub-structs
	auto drawSpawnerBaseProps = [](Scene::Components::Spawner *spawner) {
		ImGui::DragFloat("Start Time##Spawner", &spawner->startTime, 0.1f);

		ImGui::Checkbox("Is Single Burst", &spawner->frequency.isSingleBurst);
		if (spawner->frequency.isSingleBurst) {
			int c = static_cast<int>(spawner->frequency.burst.count);
			if (ImGui::DragInt("Burst Count##Spawner", &c))
				spawner->frequency.burst.count = static_cast<uint32_t>(std::max(0, c));
		} else {
			ImGui::DragFloat("Interval##Spawner", &spawner->frequency.repeating.interval, 0.1f);
			int c = static_cast<int>(spawner->frequency.repeating.maxCount);
			if (ImGui::DragInt("Max Count##Spawner", &c))
				spawner->frequency.repeating.maxCount = static_cast<uint32_t>(std::max(0, c));
		}

		int locType = static_cast<int>(spawner->location.type);
		if (ImGui::Combo("Location Type##Spawner", &locType, "Fixed\0Box\0Sphere\0")) {
			spawner->location.type = static_cast<Scene::Components::ULocation::Type>(locType);
		}

		if (locType == 0) {	 // Fixed
			ImGui::DragFloat3("Fixed Pos", &spawner->location.fixed.position.x, 0.1f);
			ImGui::DragFloat3("Fixed Euler", &spawner->location.fixed.orientationEuler.x, 0.1f);
		} else if (locType == 1) {	// Box
			ImGui::DragFloat3("Box Min##Spawner", &spawner->location.box.min.x, 0.1f);
			ImGui::DragFloat3("Box Max##Spawner", &spawner->location.box.max.x, 0.1f);
		} else if (locType == 2) {	// Sphere
			ImGui::DragFloat3("Sphere Center##Spawner", &spawner->location.sphere.center.x, 0.1f);
			ImGui::DragFloat("Sphere Radius##Spawner", &spawner->location.sphere.radius, 0.1f);
		}

		int matID = static_cast<int>(spawner->density);
		if (ImGui::InputInt("Material ID##Spawner", &matID)) spawner->density = static_cast<uint32_t>(std::max(0, matID));
	};

	if (auto *tag = ref_eM->TryGetTComponent<Scene::Components::Tag>(m_selectedEntity); tag) {
		size_t idx = tag - ref_eM->GetCompArr<Scene::Components::Tag>().Data().data();
		ImGui::TextDisabled("Tag (Idx: %zu)", idx);
		ImGui::InputText("Name", &tag->name);
	}
	ImGui::Separator();

	if (auto *tf = ref_eM->TryGetTComponent<Scene::Components::Transform>(m_selectedEntity); tf) {
		const bool isHeaderOpen =
			ImGui::CollapsingHeader(makeHeader("Transform", tf), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap);
		deferCompMenu(tf);
		if (isHeaderOpen) {
			bool changed = false;

			changed |= ImGui::DragFloat3("Position##Transform", &tf->position.x, 0.1f);

			Math::Vec3 rotDeg = Math::Degrees(Math::QuatToEuler(tf->orientation));
			if (ImGui::DragFloat3("Rotation##Transform", &rotDeg.x, 1.0f)) {
				tf->orientation = Math::EulerToQuat(Math::Radians(rotDeg));
				changed			= true;
			}

			changed |= ImGui::DragFloat3("Scale##Transform", &tf->scale.x, 0.05f);

			if (changed) tf->state = Scene::Components::Transform::TransformState::Dirty;
		}
	}

	if (auto *cam = ref_eM->TryGetTComponent<Components::Camera>(m_selectedEntity); cam) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Camera", cam), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(cam);

		if (isHeaderOpen) {
			bool changed = false;
			changed |= ImGui::Checkbox("Active", &cam->isActive);
			const char *cameraTypes[] = {"Perspective", "Orthographic"};

			int currentType = static_cast<int>(cam->type);
			if (ImGui::Combo("Camera Type##Camera", &currentType, cameraTypes, IM_ARRAYSIZE(cameraTypes))) {
				cam->type = static_cast<Components::CameraType>(currentType);
				changed	  = true;
			}

			if (cam->type == Components::CameraType::Perspective) {
				changed |= ImGui::SliderFloat("FOV##Camera", &cam->fovY, 0.1f, 179.0f);
			} else if (cam->type == Components::CameraType::Orthographic) {
				changed |= ImGui::DragFloat("Ortho Size##Camera", &cam->orthoSize, 0.1f, 0.1f, 1000.0f);
			}

			changed |= ImGui::DragFloat("Near Z##Camera", &cam->nearZ, 0.01f);
			changed |= ImGui::DragFloat("Far Z##Camera", &cam->farZ, 1.0f);

			if (changed) cam->isDirty = true;
		}
	}

	if (auto *mr = ref_eM->TryGetTComponent<Components::MeshRenderer>(m_selectedEntity); mr) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Mesh Renderer", mr), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(mr);

		if (isHeaderOpen) {
			ImGui::Checkbox("Visible", &mr->isVisible);
			ImGui::SameLine();
			ImGui::Checkbox("Transparent", &mr->forceTransparent);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Force this object to be rendered in the transparency pass (sorted back-to-front).");

			ImGui::Separator();

			ImGui::Checkbox("Cast Shadows", &mr->castShadows);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Render this object into the shadow map?");

			ImGui::Checkbox("Receive Shadows", &mr->receiveShadows);
			if (ImGui::IsItemHovered()) ImGui::SetTooltip("Should shadows be calculated on this object surface?");

			ImGui::Separator();

			ImGui::Text("SubMeshes: %d", static_cast<int>(mr->subMeshes.size()));
			ImGui::SameLine();
			if (ImGui::Button("Add SubMesh")) {
				mr->subMeshes.push_back(Components::MeshRenderer::SubMeshInfo{0, 0});
			}

			ImGui::PushID("SubMeshes");
			int subMeshToRemove = -1;
			for (int i = 0; i < mr->subMeshes.size(); ++i) {
				auto &subMesh = mr->subMeshes[i];
				ImGui::PushID(i);
				if (ImGui::TreeNode(reinterpret_cast<void *>(static_cast<intptr_t>(i)), "SubMesh %d", i)) {
					int meshID = static_cast<int>(subMesh.meshID);
					if (ImGui::InputInt("Mesh ID##SubMesh", &meshID)) {
						subMesh.meshID = static_cast<uint32_t>(std::max(0, meshID));
					}

					int materialID = static_cast<int>(subMesh.materialID);
					if (ImGui::InputInt("Material ID##SubMesh", &materialID)) {
						subMesh.materialID = static_cast<uint32_t>(std::max(0, materialID));
					}

					if (ImGui::Button("Remove SubMesh##SubMesh")) {
						subMeshToRemove = i;
					}

					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::PopID();

			if (subMeshToRemove >= 0 && subMeshToRemove < mr->subMeshes.size()) {
				mr->subMeshes.erase(mr->subMeshes.begin() + subMeshToRemove);
			}
		}
	}

	// --- PHYSICS: RIGID BODY ---
	if (auto *rb = ref_eM->TryGetTComponent<Physics::Body::Components::RigidBody>(m_selectedEntity); rb) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Rigid Body", rb), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(rb);

		if (isHeaderOpen) {
			ImGui::DragFloat3("Position##RigidBody", &rb->position.x, 0.1f);
			ImGui::DragFloat3("Velocity##RigidBody", &rb->velocity.x, 0.1f);
			ImGui::Text("Acceleration: %.2f, %.2f, %.2f", rb->acceleration.x, rb->acceleration.y, rb->acceleration.z);
			Math::Vec3 orientVec3 = Math::RQuatToREuler(rb->orientation);
			ImGui::Text("Orientation: %.2f, %.2f, %.2f", orientVec3.x, orientVec3.y, orientVec3.z);
			ImGui::DragFloat3("Angular Velocity##RigidBody", &rb->angularVelocity.x, 0.1f);
			ImGui::DragFloat("Linear Damping##RigidBody", &rb->linearDamping, 0.01f);
			ImGui::DragFloat("Angular Damping##RigidBody", &rb->angularDamping, 0.01f);
			ImGui::DragFloat("Inverse Mass##RigidBody", &rb->inverseMass, 0.01f);
			ImGui::DragFloat("Gravity Scale##RigidBody", &rb->gravityScale, 0.1f);
			ImGui::Text("Motion: %.2f", rb->motion);
			ImGui::Text("Last frame acceleration: %.2f, %.2f, %.2f", rb->lastFrameAcceleration.x,
						rb->lastFrameAcceleration.y, rb->lastFrameAcceleration.z);
			ImGui::Text("Force Accumulator: %.2f, %.2f, %.2f", rb->forceAccum.x, rb->forceAccum.y, rb->forceAccum.z);
			ImGui::Text("Torque Accumulator: %.2f, %.2f, %.2f", rb->torqueAccum.x, rb->torqueAccum.y,
						rb->torqueAccum.z);

			ImGui::Checkbox("Is Awake", &rb->isAwake);
			ImGui::SameLine();
			ImGui::Checkbox("Can Sleep", &rb->canSleep);
		}
	}

	// --- PHYSICS: BODY DRAG ---
	if (auto *comp = ref_eM->TryGetTComponent<Physics::Body::Components::Drag>(m_selectedEntity); comp) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Body Drag", comp), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(comp);
		if (isHeaderOpen) {
			ImGui::DragFloat("k1##BodyDrag", &comp->k1, 0.01f);
			ImGui::DragFloat("k2##BodyDrag", &comp->k2, 0.01f);
		}
	}

	// --- NEW PHYSICS BODY COMPONENTS ---

	if (auto *kb = ref_eM->TryGetTComponent<Physics::Body::Components::KinematicBody>(m_selectedEntity); kb) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Kinematic Body", kb), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(kb);
		if (isHeaderOpen) {
			ImGui::DragFloat3("Position##KinematicBody", &kb->position.x, 0.1f);
			ImGui::DragFloat3("Target Position##KinematicBody", &kb->targetPosition.x, 0.1f);
			Math::Vec3 orientVec3 = Math::Degrees(Math::QuatToEuler(kb->orientation));
			if (ImGui::DragFloat3("Orientation##KinematicBody", &orientVec3.x, 1.0f)) kb->orientation = Math::EulerToQuat(Math::Radians(orientVec3));
			Math::Vec3 targetOrientVec3 = Math::Degrees(Math::QuatToEuler(kb->targetOrientation));
			if (ImGui::DragFloat3("Target Orientation##KinematicBody", &targetOrientVec3.x, 1.0f)) kb->targetOrientation = Math::EulerToQuat(Math::Radians(targetOrientVec3));
			ImGui::DragFloat3("Velocity##KinematicBody", &kb->velocity.x, 0.1f);
			ImGui::DragFloat3("Angular Velocity##KinematicBody", &kb->angularVelocity.x, 0.1f);
		}
	}

	if (auto *mat = ref_eM->TryGetTComponent<Physics::Body::Components::PhysicsMaterial>(m_selectedEntity); mat) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Physics Material", mat), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(mat);
		if (isHeaderOpen) {
			int id = static_cast<int>(mat->id);
			if (ImGui::InputInt("Material ID##PhysicsMaterial", &id)) mat->id = static_cast<Physics::Body::PhysicsMaterialID>(std::max(0, id));
		}
	}

	if (auto *aero = ref_eM->TryGetTComponent<Physics::Body::Components::Aero>(m_selectedEntity); aero) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Aero", aero), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(aero);
		if (isHeaderOpen) {
			ImGui::DragFloat3("Position##Aero", &aero->position.x, 0.1f);
		}
	}

	if (auto *ac = ref_eM->TryGetTComponent<Physics::Body::Components::AeroControl>(m_selectedEntity); ac) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Aero Control", ac), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(ac);
		if (isHeaderOpen) {
			ImGui::DragFloat3("Position##AeroControl", &ac->position.x, 0.1f);
			ImGui::DragFloat("Control Setting##AeroControl", &ac->controlSetting, 0.01f);
		}
	}

	if (auto *aa = ref_eM->TryGetTComponent<Physics::Body::Components::AngledAero>(m_selectedEntity); aa) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Angled Aero", aa), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(aa);
		if (isHeaderOpen) {
			ImGui::DragFloat3("Position##AngledAero", &aa->position.x, 0.1f);
			Math::Vec3 orientVec3 = Math::Degrees(Math::QuatToEuler(aa->orientation));
			if (ImGui::DragFloat3("Orientation##AngledAero", &orientVec3.x, 1.0f)) aa->orientation = Math::EulerToQuat(Math::Radians(orientVec3));
		}
	}

	if (auto *bb = ref_eM->TryGetTComponent<Physics::Body::Components::Buoyancy>(m_selectedEntity); bb) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Body Buoyancy", bb), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(bb);
		if (isHeaderOpen) {
			ImGui::DragFloat("Max Depth##BodyBuoyancy", &bb->maxDepth, 0.1f);
			ImGui::DragFloat("Volume##BodyBuoyancy", &bb->volume, 0.1f);
			ImGui::DragFloat("Water Height##BodyBuoyancy", &bb->waterHeight, 0.1f);
			ImGui::DragFloat("Liquid Density##BodyBuoyancy", &bb->liquidDensity, 10.0f);
			ImGui::DragFloat3("Centre Of Buoyancy##BodyBuoyancy", &bb->centreOfBuoyancy.x, 0.1f);
		}
	}

	if (auto *bs = ref_eM->TryGetTComponent<Physics::Body::Components::Spring>(m_selectedEntity); bs) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Body Spring", bs), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(bs);
		if (isHeaderOpen) {
			int val = static_cast<int>(bs->otherRigidBodyIndex);
			if (ImGui::InputInt("Other RB Index##BodySpring", &val)) bs->otherRigidBodyIndex = static_cast<uint32_t>(std::max(0, val));
			ImGui::DragFloat("Spring Constant##BodySpring", &bs->springConstant, 0.1f);
			ImGui::DragFloat("Rest Length##BodySpring", &bs->restLength, 0.1f);
			ImGui::DragFloat3("Connection Point##BodySpring", &bs->connectionPoint.x, 0.1f);
			ImGui::DragFloat3("Other Connection Point##BodySpring", &bs->otherRbConnectionPoint.x, 0.1f);
		}
	}

	if (auto *bas = ref_eM->TryGetTComponent<Physics::Body::Components::AnchoredSpring>(m_selectedEntity); bas) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Body Anchored Spring", bas), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(bas);
		if (isHeaderOpen) {
			int val = static_cast<int>(bas->anchoredRigidBodyIndex);
			if (ImGui::InputInt("Anchored RB Index##BodyAnchoredSpring", &val)) bas->anchoredRigidBodyIndex = static_cast<uint32_t>(std::max(0, val));
			ImGui::DragFloat("Spring Constant##BodyAnchoredSpring", &bas->springConstant, 0.1f);
			ImGui::DragFloat("Rest Length##BodyAnchoredSpring", &bas->restLength, 0.1f);
		}
	}

	if (auto *bab = ref_eM->TryGetTComponent<Physics::Body::Components::AnchoredBungee>(m_selectedEntity); bab) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Body Anchored Bungee", bab), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(bab);
		if (isHeaderOpen) {
			int val = static_cast<int>(bab->otherRigidBodyIndex);
			if (ImGui::InputInt("Other RB Index##BodyAnchoredBungee", &val)) bab->otherRigidBodyIndex = static_cast<uint32_t>(std::max(0, val));
			ImGui::DragFloat("Spring Constant##BodyAnchoredBungee", &bab->springConstant, 0.1f);
			ImGui::DragFloat("Rest Length##BodyAnchoredBungee", &bab->restLength, 0.1f);
		}
	}

	// --- PHYSICS COLLIDERS ---
	if (auto *sc = ref_eM->TryGetTComponent<Physics::Body::Components::SphereCollider>(m_selectedEntity); sc) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("SphereCollider", sc), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(sc);

		if (isHeaderOpen) {
			ImGui::DragFloat("Radius##SphereCollider", &sc->localRadius, 0.1f);
			ImGui::DragFloat3("Local Offset##SphereCollider", &sc->localOffset.x, 0.01f);
		}
	}

	if (auto *bc = ref_eM->TryGetTComponent<Physics::Body::Components::BoxCollider>(m_selectedEntity); bc) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("BoxCollider", bc), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(bc);

		if (isHeaderOpen) {
			ImGui::DragFloat3("Half Extents##BoxCollider", &bc->localHalfExtents.x, 0.1f);
			ImGui::DragFloat3("Local Offset##BoxCollider", &bc->localOffset.x, 0.1f);
		}
	}

	if (auto *cc = ref_eM->TryGetTComponent<Physics::Body::Components::CapsuleCollider>(m_selectedEntity); cc) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("CapsuleCollider", cc), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(cc);

		if (isHeaderOpen) {
			ImGui::DragFloat("Radius##CapsuleCollider", &cc->localRadius, 0.1f);
			ImGui::DragFloat("Half Height##CapsuleCollider", &cc->localHalfHeight, 0.1f);
			ImGui::DragFloat3("Local Offset##CapsuleCollider", &cc->localOffset.x, 0.1f);
		}
	}

	if (auto *cyc = ref_eM->TryGetTComponent<Physics::Body::Components::CylinderCollider>(m_selectedEntity); cyc) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("CylinderCollider", cyc), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(cyc);

		if (isHeaderOpen) {
			ImGui::DragFloat("Radius##CylinderCollider", &cyc->localRadius, 0.1f);
			ImGui::DragFloat("Half Height##CylinderCollider", &cyc->localHalfHeight, 0.1f);
			ImGui::DragFloat3("Local Offset##CylinderCollider", &cyc->localOffset.x, 0.1f);
		}
	}

	if (auto *aabb = ref_eM->TryGetTComponent<Physics::Body::Components::AABB>(m_selectedEntity); aabb) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("AABB", aabb), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(aabb);

		if (isHeaderOpen) {
			ImGui::Text("Min: %.2f, %.2f, %.2f", aabb->min.x, aabb->min.y, aabb->min.z);
			ImGui::Text("Max: %.2f, %.2f, %.2f", aabb->max.x, aabb->max.y, aabb->max.z);

			int layer = static_cast<int>(aabb->GetCollisionLayer());
			if (ImGui::InputInt("Collision Layer##AABB", &layer))
				aabb->SetCollisionLayer(static_cast<uint32_t>(std::max(0, layer)));

			int mask = static_cast<int>(aabb->GetCollisionMask());
			if (ImGui::InputInt("Collision Mask##AABB", &mask))
				aabb->SetCollisionMask(static_cast<uint32_t>(std::max(0, mask)));
		}
	}

	if (auto *se = ref_eM->TryGetTComponent<Physics::Body::Components::StaticEntity>(m_selectedEntity); se) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Static Entity", se), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompRemoveOnly(se);
		if (isHeaderOpen) {
			ImGui::Text("This entity is tagged as static.");
		}
	}

	// --- SCENE SPAWNERS ---

	if (auto *spawner = ref_eM->TryGetTComponent<Scene::Components::Spawner>(m_selectedEntity); spawner) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Spawner", spawner), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompRemoveOnly(spawner);
		if (isHeaderOpen) {
			drawSpawnerBaseProps(spawner);
		}
	}

	// --- CORE & NETWORK ---

	if (auto *wa = ref_eM->TryGetTComponent<Scene::Components::WaypointAnimation>(m_selectedEntity); wa) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Waypoint Animation", wa), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompRemoveOnly(wa);
		if (isHeaderOpen) {
			ImGui::DragFloat("Duration##Waypoint", &wa->duration, 0.1f);

			int easing = static_cast<int>(wa->easing);
			if (ImGui::Combo("Easing##Waypoint", &easing, "Linear\0SmoothStep\0"))
				wa->easing = static_cast<Scene::Components::WaypointAnimation::EasingType>(easing);

			int mode = static_cast<int>(wa->pathMode);
			if (ImGui::Combo("Path Mode##Waypoint", &mode, "Stop\0Loop\0Reverse\0"))
				wa->pathMode = static_cast<Scene::Components::WaypointAnimation::PathMode>(mode);

			int status = static_cast<int>(wa->status);
			if (ImGui::Combo("Status##Waypoint", &status, "NotStarted\0GoingForward\0ReturningBack\0LoopingBack\0Finished\0"))
				wa->status = static_cast<Scene::Components::WaypointAnimation::Status>(status);

			ImGui::Separator();

			ImGui::Text("Waypoints: %zu", wa->waypoints.size());
			ImGui::SameLine();
			if (ImGui::Button("Add Waypoint")) {
				wa->waypoints.emplace_back(Math::Vec3(0.0f, 0.0f, 0.0f), Math::Quat(1.0f, 0.0f, 0.0f, 0.0f), 0.0f);
			}

			ImGui::PushID("Waypoints");
			int waypointToRemove = -1;
			for (int i = 0; i < wa->waypoints.size(); ++i) {
				auto &wp = wa->waypoints[i];
				ImGui::PushID(i);
				if (ImGui::TreeNode(reinterpret_cast<void *>(static_cast<intptr_t>(i)), "Waypoint %d", i)) {
					ImGui::DragFloat3("Position##WP", &wp.position.x, 0.1f);

					Math::Vec3 rotDeg = Math::Degrees(Math::QuatToEuler(wp.orientation));
					if (ImGui::DragFloat3("Rotation##WP", &rotDeg.x, 1.0f)) {
						wp.orientation = Math::EulerToQuat(Math::Radians(rotDeg));
					}

					ImGui::DragFloat("Time##WP", &wp.time, 0.1f);

					if (ImGui::Button("Remove Waypoint")) {
						waypointToRemove = i;
					}
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::PopID();

			if (waypointToRemove >= 0 && waypointToRemove < wa->waypoints.size()) {
				wa->waypoints.erase(wa->waypoints.begin() + waypointToRemove);
			}
		}
	}

	// --- LIGHT & PARTICLE SYSTEMS (Preserved) ---
	if (auto *l = ref_eM->TryGetTComponent<Components::DirectionalLight>(m_selectedEntity); l) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Directional Light", l), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(l);
		ImGui::ColorEdit4("Color##DirectionalLight", &l->color.x);
	}

	if (auto *emitter = ref_eM->TryGetTComponent<Components::ParticleEmitter>(m_selectedEntity); emitter) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Particle Emitter", emitter), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(emitter);

		if (isHeaderOpen) {
			const char *particleTypes[]	 = {"Fire", "Rain", "Snow", "Dust", "Custom"};
			int			currentTypeIndex = static_cast<int>(emitter->type);
			if (ImGui::Combo("Type##ParticleEmitter", &currentTypeIndex, particleTypes, IM_ARRAYSIZE(particleTypes))) {
				emitter->type = static_cast<ParticleType>(currentTypeIndex);
			}

			ImGui::Separator();

			int maxP = static_cast<int>(emitter->maxParticles);
			if (ImGui::DragInt("Max Particles##ParticleEmitter", &maxP, 10, 0, 100000)) {
				if (maxP < 0) maxP = 0;
				emitter->maxParticles = static_cast<uint32_t>(maxP);
			}

			ImGui::DragFloat("Spawn Rate##ParticleEmitter", &emitter->spawnRate, 1.0f, 0.1f, 1000.0f, "%.1f / sec");
			ImGui::DragFloat("Life Time##ParticleEmitter", &emitter->lifeTime, 0.1f, 0.1f, 20.0f, "%.2f sec");
			ImGui::DragFloat("Spawn Radius##ParticleEmitter", &emitter->spawnRadius, 0.5f, 0.0f, 100.0f);
			ImGui::Text("Velocity Variance");
			ImGui::DragFloat3("##VelVarParticleEmitter", &emitter->velocityVar.x, 0.1f, 0.0f, 10.0f);

			ImGui::Separator();

			int texID = static_cast<int>(emitter->textureID);
			if (ImGui::InputInt("Texture ID##ParticleEmitter", &texID)) {
				emitter->textureID = static_cast<Graphics::TextureID>(texID);
			}

			ImGui::Separator();

			ImGui::TextDisabled("Runtime Stats");
			float occupancy = (float)emitter->particles.size() / (float)emitter->maxParticles;
			char  overlay[32];
			snprintf(overlay, sizeof(overlay), "%d / %d", static_cast<int>(emitter->particles.size()), emitter->maxParticles);
			ImGui::ProgressBar(occupancy, ImVec2(0.0f, 0.0f), overlay);
		}
	}

	if (auto *dnc = ref_eM->TryGetTComponent<Scene::Components::DayNightCycle>(m_selectedEntity); dnc) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Day/Night Cycle", dnc), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(dnc);

		if (isHeaderOpen) {
			ImGui::Text("Time & Season");
			ImGui::DragFloat("Time of Day##DayNight", &dnc->timeOfDay, 0.1f, 0.0f, 24.0f, "%.2f h");
			ImGui::DragFloat("Day Duration##DayNight", &dnc->dayDuration, 1.0f, 1.0f, 600.0f, "%.1f sec");

			const char *seasonNames[] = {"Spring", "Summer", "Autumn", "Winter"};
			int			currentSeason = static_cast<int>(dnc->currentSeason);
			if (ImGui::Combo("Season##DayNight", &currentSeason, seasonNames, IM_ARRAYSIZE(seasonNames))) {
				dnc->currentSeason = static_cast<Scene::Components::Season>(currentSeason);
			}
			ImGui::ProgressBar(dnc->seasonTimer / dnc->seasonDuration, ImVec2(0.0f, 0.0f));

			ImGui::Separator();

			ImGui::Text("Atmosphere Colors");
			ImGui::ColorEdit4("Day Color##DayNight", &dnc->dayColor.x);
			ImGui::ColorEdit4("Dawn Color##DayNight", &dnc->dawnColor.x);
			ImGui::ColorEdit4("Night Color##DayNight", &dnc->nightColor.x);
			ImGui::ColorEdit4("Moon Color##DayNight", &dnc->moonColor.x);

			ImGui::Separator();

			ImGui::TextDisabled("Linked Entities (ID)");
			ImGui::Text("Sun: %d", dnc->sunEntity);
			ImGui::Text("Moon: %d", dnc->moonEntity);
			ImGui::Text("Weather: %d", dnc->weatherEntity);
		}
	}

	// --- NEW PHYSICS PARTICLE COMPONENTS ---

	if (auto *pm = ref_eM->TryGetTComponent<Physics::Particle::Components::PointMass>(m_selectedEntity); pm) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Point Mass", pm), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(pm);
		if (isHeaderOpen) {
			ImGui::DragFloat3("Position##PointMass", &pm->position.x, 0.1f);
			ImGui::DragFloat3("Velocity##PointMass", &pm->velocity.x, 0.1f);
			ImGui::Text("Acceleration: %.2f, %.2f, %.2f", pm->acceleration.x, pm->acceleration.y, pm->acceleration.z);
			ImGui::DragFloat("Linear Damping##PointMass", &pm->linearDamping, 0.01f);
			ImGui::DragFloat("Inverse Mass##PointMass", &pm->inverseMass, 0.01f);
			ImGui::DragFloat("Gravity Scale##PointMass", &pm->gravityScale, 0.1f);
		}
	}

	if (auto *comp = ref_eM->TryGetTComponent<Physics::Particle::Components::AnchoredSpring>(m_selectedEntity); comp) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Anchored Spring", comp), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(comp);
		if (isHeaderOpen) {
			int val = static_cast<int>(comp->anchoredPointMassIndex);
			if (ImGui::InputInt("Anchored PM Index##PAnchoredSpring", &val))
				comp->anchoredPointMassIndex = static_cast<uint32_t>(std::max(0, val));
			ImGui::DragFloat("Spring Constant##PAnchoredSpring", &comp->springConstant, 0.1f);
			ImGui::DragFloat("Rest Length##PAnchoredSpring", &comp->restLength, 0.1f);
		}
	}

	if (auto *comp = ref_eM->TryGetTComponent<Physics::Particle::Components::AnchoredBungee>(m_selectedEntity); comp) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Bungee Spring", comp), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(comp);
		if (isHeaderOpen) {
			int val = static_cast<int>(comp->otherPointMassIndex);
			if (ImGui::InputInt("Other PM Index##PAnchoredBungee", &val))
				comp->otherPointMassIndex = static_cast<uint32_t>(std::max(0, val));
			ImGui::DragFloat("Spring Constant##PAnchoredBungee", &comp->springConstant, 0.1f);
			ImGui::DragFloat("Rest Length##PAnchoredBungee", &comp->restLength, 0.1f);
		}
	}

	if (auto *comp = ref_eM->TryGetTComponent<Physics::Particle::Components::Buoyancy>(m_selectedEntity); comp) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Buoyancy", comp), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(comp);
		if (isHeaderOpen) {
			ImGui::DragFloat("Max Depth##PBuoyancy", &comp->maxDepth, 0.1f);
			ImGui::DragFloat("Volume##PBuoyancy", &comp->volume, 0.1f);
			ImGui::DragFloat("Water Height##PBuoyancy", &comp->waterHeight, 0.1f);
			ImGui::DragFloat("Liquid Density##PBuoyancy", &comp->liquidDensity, 10.0f);
		}
	}

	if (auto *comp = ref_eM->TryGetTComponent<Physics::Particle::Components::CableConstraint>(m_selectedEntity);
		comp) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Cable Constraint", comp), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(comp);
		if (isHeaderOpen) {
			int indices[2] = {static_cast<int>(comp->pointMassIndexes[0]), static_cast<int>(comp->pointMassIndexes[1])};
			if (ImGui::InputInt2("PM Indexes##Cable", indices)) {
				comp->pointMassIndexes[0] = static_cast<uint32_t>(std::max(0, indices[0]));
				comp->pointMassIndexes[1] = static_cast<uint32_t>(std::max(0, indices[1]));
			}
			ImGui::DragFloat("Max Length##Cable", &comp->MaxLength, 0.1f);
			ImGui::DragFloat("Restitution##Cable", &comp->Restitution, 0.01f, 0.0f, 1.0f);
		}
	}

	if (auto *comp = ref_eM->TryGetTComponent<Physics::Particle::Components::Drag>(m_selectedEntity); comp) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Drag", comp), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(comp);
		if (isHeaderOpen) {
			ImGui::DragFloat("k1##PDrag", &comp->k1, 0.01f);
			ImGui::DragFloat("k2##PDrag", &comp->k2, 0.01f);
		}
	}

	if (auto *comp = ref_eM->TryGetTComponent<Physics::Particle::Components::RodConstraint>(m_selectedEntity); comp) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Rod Constraint", comp), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(comp);
		if (isHeaderOpen) {
			int indices[2] = {static_cast<int>(comp->pointMassIndexes[0]), static_cast<int>(comp->pointMassIndexes[1])};
			if (ImGui::InputInt2("PM Indexes##Rod", indices)) {
				comp->pointMassIndexes[0] = static_cast<uint32_t>(std::max(0, indices[0]));
				comp->pointMassIndexes[1] = static_cast<uint32_t>(std::max(0, indices[1]));
			}
			ImGui::DragFloat("Length##Rod", &comp->Length, 0.1f);
		}
	}

	if (auto *comp = ref_eM->TryGetTComponent<Physics::Particle::Components::Spring>(m_selectedEntity); comp) {
		const bool isHeaderOpen = ImGui::CollapsingHeader(makeHeader("Spring", comp), ImGuiTreeNodeFlags_DefaultOpen);
		deferCompMenu(comp);
		if (isHeaderOpen) {
			int val = static_cast<int>(comp->otherPointMassIndex);
			if (ImGui::InputInt("Other PM Index##PSpring", &val))
				comp->otherPointMassIndex = static_cast<uint32_t>(std::max(0, val));
			ImGui::DragFloat("Spring Constant##PSpring", &comp->springConstant, 0.1f);
			ImGui::DragFloat("Rest Length##PSpring", &comp->restLength, 0.1f);
		}
	}

	// Safely execute all queued component removals at the end of the frame
	for (auto& action : deferredActions) {
		action();
	}
}

void GUISystem::DrawAssetBrowser() {
	if (!ImGui::Begin("Asset Manager")) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginTabBar("AssetTabs")) {
		if (ImGui::BeginTabItem("Textures")) {
			const auto &registry = Assets::AssetManager::GetTextureRegistry();
			ImGui::Text("Count: %zu", registry.size());

			if (ImGui::BeginTable("TexTable", 4,
								  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
									  ImGuiTableFlags_ScrollY)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Dimensions");
				ImGui::TableSetupColumn("Type");
				ImGui::TableHeadersRow();

				for (const auto &[name, info] : registry) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%u", info->ref_handle);

					ImGui::TableSetColumnIndex(1);
					ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
					if (ImGui::IsItemHovered() && !info->sourcePaths.empty()) {
						ImGui::SetTooltip("Source: %s", info->sourcePaths[0].string().c_str());
					}

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%dx%d", info->params.width, info->params.height);

					ImGui::TableSetColumnIndex(3);

					ImGui::TextUnformatted(PE::Utilities::EnumToString(TEX_TYPE_MAP, info->params.type).data());
				}
				ImGui::EndTable();
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Materials")) {
			const auto &registry = Assets::AssetManager::GetMaterialRegistry();
			ImGui::Text("Count: %zu", registry.size());

			if (ImGui::BeginTable("MatTable", 3,
								  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Shader Used");
				ImGui::TableHeadersRow();

				for (const auto &[name, info] : registry) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%u", info->ref_handle);

					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(name.c_str());

					ImGui::TableSetColumnIndex(2);
					ImGui::TextUnformatted(info->shaderAssetName.c_str());
				}
				ImGui::EndTable();
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Meshes")) {
			const auto &registry = Assets::AssetManager::GetMeshRegistry();
			ImGui::Text("Count: %zu", registry.size());

			if (ImGui::BeginTable("MeshTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Verts");
				ImGui::TableSetupColumn("Indices");
				ImGui::TableHeadersRow();

				for (const auto &[name, info] : registry) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%u", info->ref_handle);

					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(name.c_str());

					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%u", info->vertexCount);

					ImGui::TableSetColumnIndex(3);
					ImGui::Text("%u", info->indexCount);
				}
				ImGui::EndTable();
			}
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Shaders")) {
			const auto &registry = Assets::AssetManager::GetShaderRegistry();
			if (ImGui::BeginTable("ShaderTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
				ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Type");
				ImGui::TableHeadersRow();

				for (const auto &[name, info] : registry) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%u", info->ref_handle);
					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(name.c_str());

					ImGui::TableSetColumnIndex(2);

					ImGui::TextUnformatted(PE::Utilities::EnumToString(SHADER_TYPE_MAP, info->shaderType).data());
				}
				ImGui::EndTable();
			}
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGui::End();
}

void GUISystem::DrawPerformanceStats(float dt) {
	static float lastUpdateTime = 0.0f;
	static int	 frameCount		= 0;
	static float currentFPS		= 0.0f;
	static float currentFrameMs = 0.0f;

	static float frameTimeHistory[60] = {};
	static int	 historyOffset		  = 0;

	frameCount++;
	const float currentTime = m_fpsTimer.TotalTime();

	if (const float timeDiff = currentTime - lastUpdateTime; timeDiff >= 0.1f) {
		currentFPS	   = static_cast<float>(frameCount) / timeDiff;
		currentFrameMs = (timeDiff * 1000.0f) / static_cast<float>(frameCount);

		frameTimeHistory[historyOffset] = currentFrameMs;
		historyOffset					= (historyOffset + 1) % 60;

		lastUpdateTime = currentTime;
		frameCount	   = 0;
	}

	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowBgAlpha(0.5f);

	if (ImGui::Begin("Engine Stats", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "FPS: %.1f", currentFPS);
		ImGui::Text("Frame Time: %.3f ms", currentFrameMs);

		ImGui::PlotLines("##FrameTime", frameTimeHistory, 60, historyOffset, nullptr, 0.0f, 33.3f, ImVec2(200, 40));

		ImGui::Separator();

		RenderStats stats = ref_renderer->GetStats();

		ImGui::Text("Draw Calls:    %u", stats.drawCalls);

		float triM	= stats.triangleCount / 1000000.0f;
		float vertM = stats.vertexCount / 1000000.0f;

		ImGui::Text("Total Tris:    %.2f M", triM);
		ImGui::Text("Total Verts:   %.2f M", vertM);

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Includes all passes (Shadow + Main Render)");
		}

		ImGui::Separator();

		ImGui::Text("Uptime: %.1f s", currentTime);

#if defined(PE_VULKAN)
		ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Backend: Vulkan");
#elif defined(PE_DX11)
		ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "Backend: DirectX 11");
#endif
	}
	ImGui::End();
}

void GUISystem::DrawTopBar() {
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Scenes")) {
			static std::vector<std::filesystem::path> scenePaths =
				Utilities::IOUtilities::GetFilenamesInDirectoryRecursiveByExtension(
					Utilities::IOUtilities::GetAssetsRoot().string(), ".ini");

			for (const std::filesystem::path &sceneName : scenePaths) {
				std::string filenameStr = sceneName.filename().string();
				if (ImGui::MenuItem(filenameStr.c_str())) {
					ref_sceneManager->ResetScene();
					ref_sceneLoader->LoadScene(Utilities::IOUtilities::GetAssetPath(sceneName.string()));
					if (auto &cameraCompArr = ref_eM->GetCompArr<Components::Camera>(); cameraCompArr.GetCount() > 0)
						ref_sceneManager->SelectControlledEntity(cameraCompArr.Index()[0]);

					m_selectedEntity = ECS::INVALID_ENTITY_ID;
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Refresh Scenario List")) {
				scenePaths = Utilities::IOUtilities::GetFilenamesInDirectoryRecursiveByExtension(
					Utilities::IOUtilities::GetAssetsRoot().string(), ".scene");
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Physics")) {
			constexpr ImVec4 red(1.0f, 0.0f, 0.0f, 0.6f);
			constexpr ImVec4 green(0.0f, 1.0f, 0.0f, 0.6f);
			if (m_shouldPhysicsStop) {
				ImGui::PushStyleColor(ImGuiCol_Button, green);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, red);
				if (ImGui::Button("Start Physics")) {
					m_shouldPhysicsStop = false;
					ref_physicsSystem->TogglePhysics();
				}
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button, red);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, green);
				if (ImGui::Button("Stop Physics")) {
					m_shouldPhysicsStop = true;
					ref_physicsSystem->TogglePhysics();
				}
			}
			ImGui::PopStyleColor(2);
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}
}  // namespace PE::Graphics::Systems