#include "Graphics/Systems/RenderSystem.h"

#include "Assets/AssetManager.h"
#include "Graphics/Components/Camera.h"
#include "Graphics/Components/DirectionalLight.h"
#include "Graphics/Components/MeshRenderer.h"
#include "Graphics/D3D11/D3D11Renderer.h"
#include "Graphics/Systems/CameraSystem.h"
#include "Graphics/Vulkan/VulkanRenderer.h"
#include "Scene/Components/DayNightCycle.h"
#include "Scene/Components/Transform.h"
#include "Utilities/Logger.h"
#include "Utilities/MemoryUtilities.h"

namespace PE::Graphics::Systems {
ERROR_CODE RenderSystem::Initialize(const ECS::ESystemStage stage, ECS::EntityManager *entityManager,
									CameraSystem *cameraSystem, GLFWwindow *window, Core::EngineConfig &config) {
	PE_CHECK_STATE_INIT(m_state, "Render system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID		  = GetUniqueISystemTypeID<RenderSystem>();
	ref_entityManager = entityManager;
	ref_cameraSystem  = cameraSystem;
	m_stage			  = stage;
	ref_engineConfig  = &config;
	ref_renderConfig  = &config.renderConfig;

	ERROR_CODE result;
	PE_CHECK(result, ref_entityManager->RegisterSystem(this));
	PE_CHECK(result, InitializeRenderer(window, config));

	m_state = SystemState::Running;
	return result;
}

ERROR_CODE RenderSystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	ERROR_CODE result;
	PE_CHECK(result, Utilities::SafeShutdownReturnsErrorCode(m_renderer));
	PE_CHECK(result, ref_entityManager->UnregisterSystem(this));

	m_stage	 = ECS::ESystemStage::Count;
	m_typeID = UINT32_MAX;

	m_state = SystemState::Uninitialized;
	return result;
}

void RenderSystem::OnUpdate(float dt) {
	bool shouldFlush = false;

	ref_activeCamEntityID = ref_cameraSystem->GetActiveCameraEntityID();
	if (ref_activeCamEntityID == UINT32_MAX) return;

	const auto &cam			 = ref_entityManager->GetCompArr<Components::Camera>().Get(ref_activeCamEntityID);
	const auto &camTransform = ref_entityManager->GetCompArr<Scene::Components::Transform>().Get(ref_activeCamEntityID);

	ECS::EntityID activeLightID	  = ECS::INVALID_ENTITY_ID;
	bool		  isDayNightLight = false;

	if (auto &dncArr = ref_entityManager->GetCompArr<Scene::Components::DayNightCycle>(); dncArr.GetCount() > 0) {
		activeLightID = dncArr.Data()[0].activeLightEntity;
		if (activeLightID != ECS::INVALID_ENTITY_ID) {
			isDayNightLight = true;
		}
	}

	if (activeLightID == ECS::INVALID_ENTITY_ID) {
		if (auto &lights = ref_entityManager->GetCompArr<Components::DirectionalLight>(); lights.GetCount() > 0) {
			activeLightID	= lights.Index()[0];
			isDayNightLight = false;
		}
	}

	Components::DirectionalLight dirLightData;
	auto						 positionOfDirLight = Math::Vec3(0, 100, 0);

	if (activeLightID != ECS::INVALID_ENTITY_ID) {
		if (auto *light = ref_entityManager->TryGetTIComponent<Components::DirectionalLight>(activeLightID)) {
			dirLightData = *light;
		}

		if (auto *tf = ref_entityManager->TryGetTIComponent<Scene::Components::Transform>(activeLightID)) {
			positionOfDirLight = tf->position;
		}
	} else {
		dirLightData.color = Math::Vec4(1.0f, 0.0f, 1.0f, 1.0f);
	}

	Math::Vec3 target(0.0f, 0.0f, 0.0f);
	Math::Vec3 directionToTarget = Math::Normalize(target - positionOfDirLight);
	Math::Vec4 finalLightDir	 = Math::Vec4(directionToTarget, 0.0f);

	Graphics::CBPerPass perPassData{
		.view			   = cam.viewMatrix,
		.projection		   = cam.projectionMatrix,
		.inverseView	   = Math::Inverse(cam.viewMatrix),
		.inverseProjection = Math::Inverse(cam.projectionMatrix),
		.time			   = 0,
		.deltaTime		   = dt,
		.resolution		   = Math::Vec2(ref_renderConfig->width, ref_renderConfig->height),
		.inverseResolution = Math::Vec2(1 / ref_renderConfig->width, 1 / ref_renderConfig->height),
		._pad0			   = {},
		.lightColor		   = dirLightData.color,
		.lightDirection	   = finalLightDir,
		.ambientLightColor = Math::Vec4(0.1f, 0.1f, 0.15f, 1.0f)};

	m_renderer->UpdateGlobalBuffer(perPassData);

	auto &modelArr	   = ref_entityManager->GetCompArr<Components::MeshRenderer>();
	auto &transformArr = ref_entityManager->GetCompArr<Scene::Components::Transform>();

	Graphics::RenderPass geoPass =
		(ref_renderConfig->renderPath == RenderPathType::Deferred) ? RenderPass::GBuffer : RenderPass::Forward;

	const auto &activeEntities = modelArr.Index();
	for (uint32_t entityID : activeEntities) {
		if (!transformArr.Has(entityID)) continue;

		shouldFlush		   = true;
		auto &meshRenderer = modelArr.Get(entityID);
		auto &transform	   = transformArr.Get(entityID);

		Math::Mat44 world = transform.worldMatrix;

		float	 dist	  = Math::Distance(transform.position, transform.position);
		uint32_t depthInt = static_cast<uint32_t>(dist * 1000.0f);

		for (const auto &[meshID, materialID] : meshRenderer.subMeshes) {
			auto const	 &mat = m_renderer->GetMaterial(materialID);
			RenderCommand cmd{};
			cmd.key			= RenderKey::Create(static_cast<uint8_t>(geoPass), mat.GetShaderID(), materialID, depthInt);
			cmd.meshID		= meshID;
			cmd.materialID	= materialID;
			cmd.worldMatrix = world;
			cmd.ownerEntityID = entityID;

			cmd.flags = RenderFlag_None;
			if (meshRenderer.isVisible) cmd.flags |= RenderFlag_Visible;
			if (meshRenderer.castShadows) cmd.flags |= RenderFlag_CastShadows;
			if (meshRenderer.receiveShadows) cmd.flags |= RenderFlag_ReceiveShadows;
			if (meshRenderer.forceTransparent) cmd.flags |= RenderFlag_ForceTransparent;

			m_renderer->Submit(cmd);
		}
	}

	if (shouldFlush) m_renderer->Flush();
}

void RenderSystem::OnResize(const RenderConfig &config) {
	float newAspect = static_cast<float>(config.width) / static_cast<float>(config.height);
	if (newAspect < 0.001f) newAspect = 1.777f;
	ref_cameraSystem->OnResize(newAspect);
	m_renderer->OnResize(config);
}

ERROR_CODE RenderSystem::InitializeRenderer(GLFWwindow *window, const Core::EngineConfig &config) {
#ifdef PE_D3D11
	if (config.renderConfig.graphicAPI == SupportedGraphicAPI::D3D11) {
		PE_LOG_INFO("Initializing D3D11 Renderer...");
		m_renderer = new D3D11::D3D11Renderer();
	}
#elif PE_VULKAN
	if (config.renderConfig.graphicAPI == SupportedGraphicAPI::Vulkan) {
		PE_LOG_INFO("Initializing Vulkan Renderer...");
		m_renderer = new Vulkan::VulkanRenderer();
	}
#endif
	else {
		PE_LOG_FATAL("Unsupported graphic API!");
		return ERROR_CODE::NOT_INITIALIZED;
	}

	ERROR_CODE result;
	PE_ENSURE_INIT_SILENT(result, m_renderer->Initialize(window, config));

	return result;
}
}  // namespace PE::Graphics::Systems