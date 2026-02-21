#include "Graphics/Systems/GUISystem.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <string.h>

#include "Assets/AssetManager.h"
#include "ECS/EntityManager.h"
#include "Graphics/Components/Camera.h"
#include "Graphics/Components/DirectionalLight.h"
#include "Graphics/Components/MeshRenderer.h"
#include "Graphics/RenderTypes.h"
#include "Scene/Components/DayNightCycle.h"
#include "Scene/Components/Tag.h"
#include "Scene/Systems/SceneControlSystem.h"
#include "Utilities/EnumReflection.h"
#include "Utilities/Logger.h"
#include "Utilities/MemoryUtilities.h"

namespace PE::Scene::Components {
enum class Season;
struct DayNightCycle;
}  // namespace PE::Scene::Components

namespace PE::Graphics::Components {
struct DirectionalLight;
}

namespace PE::Graphics::Systems {
ERROR_CODE GUISystem::Initialize(const ECS::ESystemStage stage, ECS::EntityManager *entityManager,
								 Scene::Systems::SceneControlSystem *sceneControlSystem, IRenderer *renderer) {
	PE_CHECK_STATE_INIT(m_state, "GUI system is already initialized!");
	m_state = SystemState::Initializing;

	m_typeID	 = GetUniqueISystemTypeID<GUISystem>();
	m_stage		 = stage;
	ref_eM		 = entityManager;
	ref_renderer = renderer;

	m_fpsTimer = new Utilities::Timer();
	ERROR_CODE result;

	PE_CHECK(result, ref_renderer->InitGUI());

	PE_LOG_INFO("GUI System Initialized.");
	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

ERROR_CODE GUISystem::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	Utilities::SafeDelete(m_fpsTimer);

	m_state = SystemState::Uninitialized;
	return ERROR_CODE::OK;
}

void GUISystem::OnUpdate(float dt) {
	if (m_state != SystemState::Running) return;

	ref_renderer->NewFrameGUI();

	if (m_shouldRender) {
		DrawPerformanceStats(dt);
		DrawHierarchy();
		DrawInspector();
		DrawAssetBrowser();

		if (m_showDemoWindow) {
			ImGui::ShowDemoWindow(&m_showDemoWindow);
		}
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
										const std::unordered_map<uint32_t, std::vector<uint32_t>> &childMap) {
	std::string name = "Entity " + std::to_string(entityID);
	if (auto *tag = ref_eM->TryGetTIComponent<PE::Scene::Components::Tag>(entityID)) {
		if (!tag->name.empty()) name = tag->name;
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (m_selectedEntity == entityID) flags |= ImGuiTreeNodeFlags_Selected;

	bool hasChildren = childMap.find(entityID) != childMap.end();
	if (!hasChildren) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	bool opened = ImGui::TreeNodeEx((void *)(uint64_t)entityID, flags, "%s", name.c_str());

	if (ImGui::IsItemClicked()) {
		m_selectedEntity = entityID;
	}

	if (opened && hasChildren) {
		for (uint32_t childID : childMap.at(entityID)) {
			DrawEntityNodeRecursive(childID, childMap);
		}
		ImGui::TreePop();
	}
}

void GUISystem::DrawInspector() {
	if (!ImGui::Begin("Inspector")) {
		ImGui::End();
		return;
	}

	if (m_selectedEntity != ECS::INVALID_ENTITY_ID) {
		if (auto *tag = ref_eM->TryGetTIComponent<Scene::Components::Tag>(m_selectedEntity)) {
			ImGui::InputText("Name", &tag->name);
		}
		ImGui::Separator();

		if (auto *tf = ref_eM->TryGetTIComponent<PE::Scene::Components::Transform>(m_selectedEntity)) {
			if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
				bool changed = false;

				changed |= ImGui::DragFloat3("Position", &tf->position.x, 0.1f);

				Math::Vector3 rotDeg = Math::Vec3Degrees(tf->rotation);
				if (ImGui::DragFloat3("Rotation", &rotDeg.x, 1.0f)) {
					tf->rotation = Math::Vec3Radians(rotDeg);
					changed		 = true;
				}

				changed |= ImGui::DragFloat3("Scale", &tf->scale.x, 0.05f);

				if (changed) tf->state = PE::Scene::Components::Transform::TransformState::Dirty;
			}
		}

		if (auto *cam = ref_eM->TryGetTIComponent<PE::Graphics::Components::Camera>(m_selectedEntity)) {
			if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
				bool changed = false;
				changed |= ImGui::Checkbox("Active", &cam->isActive);
				changed |= ImGui::SliderFloat("FOV", &cam->fovY, 0.1f, 3.14f);
				changed |= ImGui::DragFloat("Near Z", &cam->nearZ, 0.01f);
				changed |= ImGui::DragFloat("Far Z", &cam->farZ, 1.0f);

				if (changed) cam->isDirty = true;
			}
		}

		if (auto *mr = ref_eM->TryGetTIComponent<Components::MeshRenderer>(m_selectedEntity)) {
			if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Checkbox("Visible", &mr->isVisible);

				ImGui::SameLine();
				ImGui::Checkbox("Transparent", &mr->forceTransparent);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(
						"Force this object to be rendered in the transparency pass (sorted back-to-front).");

				ImGui::Separator();

				ImGui::Checkbox("Cast Shadows", &mr->castShadows);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Render this object into the shadow map?");

				ImGui::Checkbox("Receive Shadows", &mr->receiveShadows);
				if (ImGui::IsItemHovered()) ImGui::SetTooltip("Should shadows be calculated on this object surface?");

				ImGui::Separator();

				ImGui::Text("SubMeshes: %d", (int)mr->subMeshes.size());
				for (int i = 0; i < mr->subMeshes.size(); ++i) {
					auto &submesh = mr->subMeshes[i];
					ImGui::PushID(i);
					if (ImGui::TreeNode((void *)(intptr_t)i, "SubMesh %d", i)) {
						ImGui::Text("Mesh ID: %d", (int)submesh.meshID);
						ImGui::Text("Material ID: %d", (int)submesh.materialID);
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
		}

		if (auto *l = ref_eM->TryGetTIComponent<Components::DirectionalLight>(m_selectedEntity)) {
			if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::ColorEdit4("Color", &l->color.x);
			}
		}

		if (auto *emitter = ref_eM->TryGetTIComponent<Components::ParticleEmitter>(m_selectedEntity)) {
			if (ImGui::CollapsingHeader("Particle Emitter", ImGuiTreeNodeFlags_DefaultOpen)) {
				const char *particleTypes[]	 = {"Fire", "Rain", "Snow", "Dust", "Custom"};
				int			currentTypeIndex = static_cast<int>(emitter->type);
				if (ImGui::Combo("Type", &currentTypeIndex, particleTypes, IM_ARRAYSIZE(particleTypes))) {
					emitter->type = static_cast<ParticleType>(currentTypeIndex);
				}

				ImGui::Separator();

				int maxP = static_cast<int>(emitter->maxParticles);
				if (ImGui::DragInt("Max Particles", &maxP, 10, 0, 100000)) {
					if (maxP < 0) maxP = 0;
					emitter->maxParticles = static_cast<uint32_t>(maxP);
				}

				ImGui::DragFloat("Spawn Rate", &emitter->spawnRate, 1.0f, 0.1f, 1000.0f, "%.1f / sec");
				ImGui::DragFloat("Life Time", &emitter->lifeTime, 0.1f, 0.1f, 20.0f, "%.2f sec");
				ImGui::DragFloat("Spawn Radius", &emitter->spawnRadius, 0.5f, 0.0f, 100.0f);
				ImGui::Text("Velocity Variance");
				ImGui::DragFloat3("##VelVar", &emitter->velocityVar.x, 0.1f, 0.0f, 10.0f);

				ImGui::Separator();

				int texID = static_cast<int>(emitter->textureID);
				if (ImGui::InputInt("Texture ID", &texID)) {
					emitter->textureID = static_cast<Graphics::TextureID>(texID);
				}

				ImGui::Separator();

				ImGui::TextDisabled("Runtime Stats");
				float occupancy = (float)emitter->particles.size() / (float)emitter->maxParticles;
				char  overlay[32];
				snprintf(overlay, sizeof(overlay), "%d / %d", (int)emitter->particles.size(), emitter->maxParticles);
				ImGui::ProgressBar(occupancy, ImVec2(0.0f, 0.0f), overlay);
			}
		}

		if (auto *dnc = ref_eM->TryGetTIComponent<Scene::Components::DayNightCycle>(m_selectedEntity)) {
			if (ImGui::CollapsingHeader("Day/Night Cycle", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::Text("Time & Season");
				ImGui::DragFloat("Time of Day", &dnc->timeOfDay, 0.1f, 0.0f, 24.0f, "%.2f h");
				ImGui::DragFloat("Day Duration", &dnc->dayDuration, 1.0f, 1.0f, 600.0f, "%.1f sec");

				const char *seasonNames[] = {"Spring", "Summer", "Autumn", "Winter"};
				int			currentSeason = static_cast<int>(dnc->currentSeason);
				if (ImGui::Combo("Season", &currentSeason, seasonNames, IM_ARRAYSIZE(seasonNames))) {
					dnc->currentSeason = static_cast<Scene::Components::Season>(currentSeason);
				}
				ImGui::ProgressBar(dnc->seasonTimer / dnc->seasonDuration, ImVec2(0.0f, 0.0f));

				ImGui::Separator();

				ImGui::Text("Atmosphere Colors");
				ImGui::ColorEdit4("Day Color", &dnc->dayColor.x);
				ImGui::ColorEdit4("Dawn Color", &dnc->dawnColor.x);
				ImGui::ColorEdit4("Night Color", &dnc->nightColor.x);
				ImGui::ColorEdit4("Moon Color", &dnc->moonColor.x);

				ImGui::Separator();

				ImGui::TextDisabled("Linked Entities (ID)");
				ImGui::Text("Sun: %d", dnc->sunEntity);
				ImGui::Text("Moon: %d", dnc->moonEntity);
				ImGui::Text("Weather: %d", dnc->weatherEntity);
			}
		}
	} else {
		ImGui::Text("Select an entity from Hierarchy.");
	}

	ImGui::End();
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
					ImGui::Selectable(info->name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
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
					ImGui::TextUnformatted(info->name.c_str());

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
					ImGui::TextUnformatted(info->name.c_str());

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
					ImGui::TextUnformatted(info->name.c_str());

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
	const float currentTime = m_fpsTimer->TotalTime();

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
}  // namespace PE::Graphics::Systems