#include "Graphics/Systems/CameraSystem.h"

#include "Assets/AssetInfo.h"
#include "Graphics/Components/Camera.h"

namespace PE::Graphics::Systems {
ERROR_CODE CameraSystem::Initialize(const ECS::ESystemStage stage, ECS::ECSManager *ecsManager,
									Input::InputSystem *inputSystem) {
	PE_CHECK_STATE_INIT(m_state, "Render system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID		= GetUniqueISystemTypeID<CameraSystem>();
	m_stage			= stage;
	ref_eM			= ecsManager;
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
	bool isActiveCameraValid = false;

	auto		&cameras	   = ref_eM->GetCompArr<Components::Camera>();
	auto		&cameraData	   = cameras.Data();
	const auto	&entityIndices = cameras.Index();
	const size_t count		   = cameras.GetCount();

	for (size_t i = 0; i < count; ++i) {
		auto &cam = cameraData[i];

		const uint32_t entityID = entityIndices[i];
		if (entityID == m_activeCamera)
			isActiveCameraValid = true;
		if (cam.isDirty) {
			if (cam.type == Components::CameraType::Perspective) {
				cam.projectionMatrix = Math::Perspective(Math::Radians(cam.fovY), cam.aspectRatio, cam.nearZ, cam.farZ);
			} else if (cam.type == Components::CameraType::Orthographic) {
				const float halfHeight = cam.orthoSize * 0.5f;
				const float halfWidth  = halfHeight * cam.aspectRatio;

				cam.projectionMatrix =
					Math::Mat4Ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, cam.nearZ, cam.farZ);
			}
			cam.isDirty = false;
		}

		if (const auto *tfComp = ref_eM->TryGetTComponent<Scene::Components::Transform>(entityID)) {
			if (tfComp->state == Scene::Components::Transform::TransformState::Updated) {
				cam.viewMatrix = UpdateViewMatrix(*tfComp);
			}
		}
	}

	if (!isActiveCameraValid)
	{
		if (count > 0)
			SelectActiveCamera(cameras.Index()[0]);
		else
			PE_LOG_FATAL("Neither is the camera active, nor is there a camera!");
	}
}

void CameraSystem::SelectActiveCamera(const ECS::EntityID activeCamID) {
	if (m_activeCamera != ECS::INVALID_ENTITY_ID && ref_eM->HasComponent<Components::Camera>(m_activeCamera))
		ref_eM->GetTComponent<Components::Camera>(m_activeCamera)->isActive = false;

	m_activeCamera														 = activeCamID;
	ref_eM->GetTComponent<Components::Camera>(m_activeCamera)->isActive = true;
}

void CameraSystem::MarkDirty(const ECS::EntityID entityID) const {
	ref_eM->GetTComponent<Components::Camera>(entityID)->isDirty = true;
}

void CameraSystem::OnResize(const float aspectRatio) const {
	for (auto &cameras = ref_eM->GetCompArr<Components::Camera>().Data(); auto &cam : cameras) {
		cam.aspectRatio = aspectRatio;

		if (cam.type == Components::CameraType::Perspective)
			cam.projectionMatrix = Math::Perspective(Math::Radians(cam.fovY), cam.aspectRatio, cam.nearZ, cam.farZ);
		else if (cam.type == Components::CameraType::Orthographic) {
			const float halfHeight = cam.orthoSize * 0.5f;
			const float halfWidth  = halfHeight * cam.aspectRatio;

			cam.projectionMatrix = Math::Mat4Ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, cam.nearZ, cam.farZ);
		}
	}
}

Math::Mat44 CameraSystem::UpdateViewMatrix(const Scene::Components::Transform &transform) const {
	return Math::Inverse(transform.worldMatrix);
}
}  // namespace PE::Graphics::Systems