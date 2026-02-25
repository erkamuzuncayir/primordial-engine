#include "Graphics/Systems/CameraSystem.h"

#include "Assets/AssetInfo.h"
#include "Graphics/Components/Camera.h"

namespace PE::Graphics::Systems {
ERROR_CODE CameraSystem::Initialize(const ECS::ESystemStage stage, ECS::EntityManager *entityManager,
									Input::InputSystem *inputSystem, const RenderConfig &renderConfig) {
	PE_CHECK_STATE_INIT(m_state, "Render system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID		= GetUniqueISystemTypeID<CameraSystem>();
	m_stage			= stage;
	ref_eM			= entityManager;
	ref_inputSystem = inputSystem;

	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

ERROR_CODE CameraSystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	m_state = SystemState::Uninitialized;
	return ERROR_CODE::OK;
}

void CameraSystem::OnUpdate(float dt) {
	auto		&cameras	   = ref_eM->GetCompArr<Components::Camera>();
	auto		&cameraData	   = cameras.Data();
	const auto	&entityIndices = cameras.Index();
	const size_t count		   = cameras.GetCount();

	for (size_t i = 0; i < count; ++i) {
		auto &cam = cameraData[i];

		const uint32_t entityID = entityIndices[i];

		if (cam.isDirty) {
			cam.projectionMatrix = Math::Perspective(Math::Radians(cam.fovY), cam.aspectRatio, cam.nearZ, cam.farZ);
			cam.isDirty			 = false;
		}

		if (const auto *tfComp = ref_eM->TryGetTIComponent<Scene::Components::Transform>(entityID)) {
			if (tfComp->state == Scene::Components::Transform::TransformState::Updated) {
				cam.viewMatrix = UpdateViewMatrix(*tfComp);
			}
		}
	}
}

void CameraSystem::SelectActiveCamera(const ECS::EntityID activeCamID) { m_activeCamera = activeCamID; }

void CameraSystem::MarkDirty(const ECS::EntityID entityID) const {
	ref_eM->GetTIComponent<Components::Camera>(entityID)->isDirty = true;
}

void CameraSystem::OnResize(const float aspectRatio) const {
	for (auto &cameras = ref_eM->GetCompArr<Components::Camera>().Data(); auto &cam : cameras) {
		cam.aspectRatio		 = aspectRatio;
		cam.projectionMatrix = Math::Perspective(Math::Radians(cam.fovY), cam.aspectRatio, cam.nearZ, cam.farZ);
	}
}

Math::Mat44 CameraSystem::UpdateViewMatrix(const Scene::Components::Transform &transform) const {
	return Math::Inverse(transform.worldMatrix);
}
}  // namespace PE::Graphics::Systems