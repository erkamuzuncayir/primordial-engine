#include "Graphics/Vulkan/VulkanRenderer.h"

#include <algorithm>

#include "Assets/AssetManager.h"
#include "Assets/Texture.h"
#include "Graphics/Vulkan/VulkanPipeline.h"
#include "Utilities/Logger.h"
#include "Utilities/MemoryUtilities.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"

namespace PE::Graphics::Vulkan {
ERROR_CODE VulkanRenderer::Initialize(GLFWwindow *windowHandle, const Core::EngineConfig &config) {
	PE_CHECK_STATE_INIT(m_state, "Vulkan renderer is already initialized.");
	m_state = SystemState::Initializing;

	ref_engineConfig = &config;
	ref_renderConfig = &config.renderConfig;
	ref_window		 = windowHandle;

	ERROR_CODE result;
	ref_device = new VulkanDevice();
	PE_ENSURE_INIT_SILENT(result, ref_device->Initialize(windowHandle));

	m_swapChain = new VulkanSwapchain();
	PE_ENSURE_INIT_SILENT(
		result, m_swapChain->Initialize(windowHandle, ref_device, ref_renderConfig->width, ref_renderConfig->height));

	m_command = new VulkanCommand();
	PE_ENSURE_INIT_SILENT(result, m_command->Initialize(ref_device, ref_renderConfig->maxFramesInFlight));

	PE_CHECK(result, CreateDepthBuffer());
	PE_ENSURE_INIT_SILENT(result, CreateSyncObjects(ref_renderConfig->maxFramesInFlight));

	PE_ENSURE_INIT_SILENT(result, CreateGlobalTextureSamplers());
	PE_ENSURE_INIT_SILENT(result, CreateShadowResources());

	PE_CHECK(result, CreateDescriptorSetLayout());
	PE_ENSURE_INIT_SILENT(result, CreateDescriptorPool());
	PE_ENSURE_INIT_SILENT(result, CreateVertexBuffer());
	PE_ENSURE_INIT_SILENT(result, CreateIndexBuffer());
	PE_ENSURE_INIT_SILENT(result, CreateUniformBuffers(ref_engineConfig->maxEntityCount));
	PE_ENSURE_INIT_SILENT(result, CreateDescriptorSets());

	PE_ENSURE_INIT_SILENT(result, CreateParticleResources());
	PE_CHECK(result, CreateStandardPipelineLayout());

	PE_LOG_INFO("Vulkan Renderer Initialized.");
	m_state = SystemState::Running;
	return result;
}

ERROR_CODE VulkanRenderer::CreateDefaultResources() {
	ERROR_CODE result;
	PE_ENSURE_INIT_SILENT(result, CreateShadowPipeline());
	PE_ENSURE_INIT_SILENT(result, CreateParticlePipeline());
	return result;
}

ERROR_CODE VulkanRenderer::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;
	m_state = SystemState::ShuttingDown;

	const VkDevice device = ref_device->GetVkDevice();
	vkDeviceWaitIdle(device);

	ShutdownGUI();

	for (auto *buf : m_particleInstanceBuffers) Utilities::SafeShutdown(buf);
	m_particleInstanceBuffers.clear();

	m_renderQueue.clear();
	m_materials.Clear();

	for (auto &shader : m_shaders.Data()) shader.Shutdown();
	m_shaders.Clear();
	m_meshes.Clear();

	for (const auto &rt : m_renderTargets.Data()) {
		if (rt.imageView != VK_NULL_HANDLE) vkDestroyImageView(device, rt.imageView, nullptr);
		if (rt.image != VK_NULL_HANDLE) vkDestroyImage(device, rt.image, nullptr);
		if (rt.memory != VK_NULL_HANDLE) vkFreeMemory(device, rt.memory, nullptr);
	}
	m_renderTargets.Clear();

	for (const auto &tex : m_textures.Data()) {
		if (tex.imageView != VK_NULL_HANDLE) vkDestroyImageView(device, tex.imageView, nullptr);
		if (tex.image != VK_NULL_HANDLE) vkDestroyImage(device, tex.image, nullptr);
		if (tex.memory != VK_NULL_HANDLE) vkFreeMemory(device, tex.memory, nullptr);
	}
	m_textures.Clear();

	if (m_shadowMap.texture.imageView) vkDestroyImageView(device, m_shadowMap.texture.imageView, nullptr);
	if (m_shadowMap.texture.image) vkDestroyImage(device, m_shadowMap.texture.image, nullptr);
	if (m_shadowMap.texture.memory) vkFreeMemory(device, m_shadowMap.texture.memory, nullptr);

	for (auto &[desc, pipeline] : m_pipelineDescriptions) {
		Utilities::SafeShutdown(pipeline);
	}
	m_pipelineDescriptions.clear();
	Utilities::SafeShutdown(m_particlePipeline);
	Utilities::SafeShutdown(m_shadowPipeline);

	if (m_pipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
	if (m_particlePipelineLayout) vkDestroyPipelineLayout(ref_device->GetVkDevice(), m_particlePipelineLayout, nullptr);
	if (m_shadowPipelineLayout) vkDestroyPipelineLayout(device, m_shadowPipelineLayout, nullptr);

	if (m_depthTexture.imageView != VK_NULL_HANDLE) vkDestroyImageView(device, m_depthTexture.imageView, nullptr);
	if (m_depthTexture.image != VK_NULL_HANDLE) vkDestroyImage(device, m_depthTexture.image, nullptr);
	if (m_depthTexture.memory != VK_NULL_HANDLE) vkFreeMemory(device, m_depthTexture.memory, nullptr);

	for (const auto &sampler : m_globalSamplers) vkDestroySampler(device, sampler, nullptr);

	Utilities::SafeShutdown(m_vertexBuffer);
	Utilities::SafeShutdown(m_indexBuffer);
	for (auto &buffer : m_perPassBuffers) Utilities::SafeShutdown(buffer);
	for (auto &buffer : m_perObjectBuffers) Utilities::SafeShutdown(buffer);
	for (auto &buffer : m_perMaterialBuffers) Utilities::SafeShutdown(buffer);

	if (m_descriptorPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
	for (const auto &layout : m_descriptorSetLayouts) vkDestroyDescriptorSetLayout(device, layout, nullptr);
	if (m_particleSetLayout) vkDestroyDescriptorSetLayout(ref_device->GetVkDevice(), m_particleSetLayout, nullptr);

	for (int i = 0; i < ref_renderConfig->maxFramesInFlight; ++i) {
		vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, m_inFlightFences[i], nullptr);
	}
	for (const auto &sem : m_renderFinishedSemaphores) vkDestroySemaphore(device, sem, nullptr);

	Utilities::SafeShutdown(m_command);
	Utilities::SafeShutdown(m_swapChain);
	Utilities::SafeShutdown(ref_device);

	m_state = SystemState::Uninitialized;
	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::OnResize(const RenderConfig &config) {
	ref_renderConfig = &config;
	RecreateSwapchain(config.width, config.height);
	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::InitGUI() {
	VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
										 {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
										 {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
										 {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
										 {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
										 {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
										 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
										 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
										 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
										 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
										 {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType						 = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags						 = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets					 = 1000;
	pool_info.poolSizeCount				 = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes				 = pool_sizes;

	if (vkCreateDescriptorPool(ref_device->GetVkDevice(), &pool_info, nullptr, &m_imguiPool) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan: Failed to create ImGui descriptor pool!");
		return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(ref_window, true);

	ImGui_ImplVulkan_InitInfo init_info	   = {};
	init_info.Instance					   = ref_device->GetInstance();
	init_info.PhysicalDevice			   = ref_device->GetVkPhysicalDevice();
	init_info.Device					   = ref_device->GetVkDevice();
	init_info.QueueFamily				   = ref_device->GetQueueFamilies().graphicsFamily.value();
	init_info.Queue						   = ref_device->GetGraphicsQueue();
	init_info.PipelineCache				   = VK_NULL_HANDLE;
	init_info.DescriptorPool			   = m_imguiPool;
	init_info.MinImageCount				   = ref_renderConfig->maxFramesInFlight;
	init_info.ImageCount				   = ref_renderConfig->maxFramesInFlight;
	init_info.UseDynamicRendering		   = true;
	init_info.PipelineInfoMain.Subpass	   = 0;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.CheckVkResultFn			   = [](const VkResult err) {
		 if (err == 0) return;

		 PE_LOG_FATAL("ImGui Vulkan Error: VkResult = " + std::to_string(err));

		 if (err < 0) {
			 abort();
		 }
	};

	VkPipelineRenderingCreateInfo imguiPipelineInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
	imguiPipelineInfo.colorAttachmentCount	  = 1;
	VkFormat colorFormat					  = m_swapChain->GetImageFormat();
	imguiPipelineInfo.pColorAttachmentFormats = &colorFormat;
	imguiPipelineInfo.depthAttachmentFormat	  = m_depthTexture.format;

	init_info.PipelineInfoMain.PipelineRenderingCreateInfo = imguiPipelineInfo;

	ImGui_ImplVulkan_Init(&init_info);

	return ERROR_CODE::OK;
}

void VulkanRenderer::NewFrameGUI() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void VulkanRenderer::ShutdownGUI() {
	vkDeviceWaitIdle(ref_device->GetVkDevice());

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	if (m_imguiPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(ref_device->GetVkDevice(), m_imguiPool, nullptr);
		m_imguiPool = VK_NULL_HANDLE;
	}
}

void VulkanRenderer::Submit(const RenderCommand &cmd) { m_renderQueue.push_back(cmd); }

void VulkanRenderer::SubmitParticles(TextureID texture, const std::vector<Graphics::Components::Particle> &particles) {
	if (particles.empty()) return;

	ParticleBatch batch;
	batch.textureID = texture;
	batch.instances.reserve(particles.size());

	for (const auto &p : particles) {
		batch.instances.push_back({p.position, p.color, p.size});
	}

	m_particleBatches.push_back(std::move(batch));
}

void VulkanRenderer::Flush() {
	vkWaitForFences(ref_device->GetVkDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult vkResult = m_swapChain->AcquireNextImage(m_imageAvailableSemaphores[m_currentFrame], imageIndex);

	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR) {
		RecreateSwapchain(ref_renderConfig->width, ref_renderConfig->height);
		return;
	} else if (vkResult != VK_SUCCESS && vkResult != VK_SUBOPTIMAL_KHR) {
		PE_LOG_FATAL("Vulkan failed to acquire swapchain image!");
		return;
	}

	vkResetFences(ref_device->GetVkDevice(), 1, &m_inFlightFences[m_currentFrame]);

	std::sort(m_renderQueue.begin(), m_renderQueue.end(), [](const auto &a, const auto &b) { return a.key < b.key; });

	UpdateUniformBuffer(m_currentFrame);

	VkCommandBuffer cmd = m_command->GetCommandBuffer(m_currentFrame);
	vkResetCommandBuffer(cmd, 0);

	ERROR_CODE result = RecordCommandBuffer(imageIndex, cmd);
	if (result < ERROR_CODE::WARN_START) {
		return;
	}

	VkSemaphore			 waitSem[]	  = {m_imageAvailableSemaphores[m_currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore			 signalSem[]  = {m_renderFinishedSemaphores[imageIndex]};
	VkSubmitInfo		 submitInfo{
				.sType				  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount	  = 1,
				.pWaitSemaphores	  = waitSem,
				.pWaitDstStageMask	  = waitStages,
				.commandBufferCount	  = 1,
				.pCommandBuffers	  = &cmd,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores	  = signalSem,
	};
	if (vkQueueSubmit(ref_device->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed submitting queue!");
		return;
	}

	vkResult = m_swapChain->Present(m_renderFinishedSemaphores[imageIndex], imageIndex);
	if (vkResult == VK_ERROR_OUT_OF_DATE_KHR || vkResult == VK_SUBOPTIMAL_KHR) {
		RecreateSwapchain(ref_renderConfig->width, ref_renderConfig->height);
	} else if (vkResult != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to present swapchain image!");
	}
	m_renderQueue.clear();
	m_currentFrame = (m_currentFrame + 1) % ref_renderConfig->maxFramesInFlight;
}

void VulkanRenderer::FlushParticles(VkCommandBuffer cmd) {
	if (m_particleBatches.empty()) return;

	VulkanBuffer *instanceBuf  = m_particleInstanceBuffers[m_currentFrame];
	char		 *mappedData   = static_cast<char *>(instanceBuf->GetMappedData());
	size_t		  globalOffset = 0;

	m_particlePipeline->Bind(cmd);

	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_particlePipelineLayout, 0, 1,
							&m_perPassDescriptorSets[m_currentFrame], 0, nullptr);

	for (const auto &batch : m_particleBatches) {
		size_t batchSizeBytes = batch.instances.size() * sizeof(GPUInstanceData);

		if (globalOffset + batchSizeBytes > ref_renderConfig->maxParticlesPerFrame * sizeof(GPUInstanceData)) {
			PE_LOG_WARN("Particle buffer overflow! Dropping particles.");
			break;
		}

		memcpy(mappedData + globalOffset, batch.instances.data(), batchSizeBytes);

		VkDescriptorSet texSet = VK_NULL_HANDLE;

		auto it = m_particleTextureSets.find(batch.textureID);
		if (it != m_particleTextureSets.end()) {
			texSet = it->second;
		} else {
			if (!m_textures.Has(batch.textureID)) {
				PE_LOG_ERROR("Particle texture not found: " + std::to_string(batch.textureID));
				continue;
			}

			VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
			allocInfo.descriptorPool	 = m_descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts		 = &m_particleSetLayout;

			if (vkAllocateDescriptorSets(ref_device->GetVkDevice(), &allocInfo, &texSet) != VK_SUCCESS) {
				PE_LOG_ERROR("Failed to allocate particle descriptor set! Pool might be full.");
				continue;
			}

			VulkanTextureWrapper &tex = m_textures.Get(batch.textureID);
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView	  = tex.imageView;
			imageInfo.sampler	  = m_globalSamplers[static_cast<int>(SamplerType::LinearRepeat)];

			VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			write.dstSet		  = texSet;
			write.dstBinding	  = 0;
			write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write.descriptorCount = 1;
			write.pImageInfo	  = &imageInfo;

			vkUpdateDescriptorSets(ref_device->GetVkDevice(), 1, &write, 0, nullptr);

			m_particleTextureSets[batch.textureID] = texSet;
		}

		if (texSet != VK_NULL_HANDLE) {
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_particlePipelineLayout, 1, 1, &texSet, 0,
									nullptr);
		}

		const VulkanMeshWrapper &quad		= m_meshes.Get(Assets::AssetManager::DefaultQuadID);
		VkBuffer				 vBuffers[] = {quad.vertexBuffer, instanceBuf->GetBuffer()};
		VkDeviceSize			 vOffsets[] = {quad.firstVertex * sizeof(Vertex), globalOffset};

		vkCmdBindVertexBuffers(cmd, 0, 2, vBuffers, vOffsets);
		vkCmdBindIndexBuffer(cmd, quad.indexBuffer, quad.firstIndex * sizeof(uint32_t), VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cmd, quad.indexCount, (uint32_t)batch.instances.size(), 0, 0, 0);

		m_stats.drawCalls++;
		m_stats.triangleCount += (quad.indexCount / 3) * batch.instances.size();
		m_stats.vertexCount += quad.vertexCount * batch.instances.size();

		globalOffset += batchSizeBytes;
	}

	m_particleBatches.clear();
}

ERROR_CODE VulkanRenderer::RecordCommandBuffer(uint32_t imageIndex, VkCommandBuffer cmd) {
	m_stats = {};
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(cmd, &beginInfo) != VK_SUCCESS) return ERROR_CODE::VULKAN_BUFFER_CREATION_FAILED;

	m_currentPipeline = nullptr;

	VkImageMemoryBarrier2 shadowBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
	shadowBarrier.srcStageMask	   = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	shadowBarrier.srcAccessMask	   = 0;
	shadowBarrier.dstStageMask	   = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
	shadowBarrier.dstAccessMask	   = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	shadowBarrier.oldLayout		   = VK_IMAGE_LAYOUT_UNDEFINED;
	shadowBarrier.newLayout		   = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	shadowBarrier.image			   = m_shadowMap.texture.image;
	shadowBarrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

	VkDependencyInfo shadowDep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
	shadowDep.imageMemoryBarrierCount = 1;
	shadowDep.pImageMemoryBarriers	  = &shadowBarrier;
	vkCmdPipelineBarrier2(cmd, &shadowDep);

	VkRenderingAttachmentInfo shadowAtt{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
	shadowAtt.imageView				  = m_shadowMap.texture.imageView;
	shadowAtt.imageLayout			  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	shadowAtt.loadOp				  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	shadowAtt.storeOp				  = VK_ATTACHMENT_STORE_OP_STORE;
	shadowAtt.clearValue.depthStencil = {1.0f, 0};

	VkRenderingInfo shadowRenderInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
	shadowRenderInfo.renderArea		  = {{0, 0}, {m_shadowMap.dim, m_shadowMap.dim}};
	shadowRenderInfo.layerCount		  = 1;
	shadowRenderInfo.pDepthAttachment = &shadowAtt;

	vkCmdBeginRendering(cmd, &shadowRenderInfo);

	if (!m_renderQueue.empty()) {
		m_shadowPipeline->Bind(cmd);

		VkViewport shadowVP = {0, 0, (float)m_shadowMap.dim, (float)m_shadowMap.dim, 0.0f, 1.0f};
		vkCmdSetViewport(cmd, 0, 1, &shadowVP);
		VkRect2D shadowScissor = {{0, 0}, {m_shadowMap.dim, m_shadowMap.dim}};
		vkCmdSetScissor(cmd, 0, 1, &shadowScissor);

		float depthBiasConstant = 1.75f;
		float depthBiasSlope	= 1.25f;

		vkCmdSetDepthBias(cmd, depthBiasConstant, 0.0f, depthBiasSlope);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipelineLayout, 0, 1,
								&m_perPassDescriptorSets[m_currentFrame], 0, nullptr);

		VkBuffer	 vBuffers[] = {m_vertexBuffer->GetBuffer()};
		VkDeviceSize offsets[]	= {0};
		vkCmdBindVertexBuffers(cmd, 0, 1, vBuffers, offsets);
		vkCmdBindIndexBuffer(cmd, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		for (size_t i = 0; i < m_renderQueue.size(); i++) {
			const auto &item = m_renderQueue[i];

			if (!(item.flags & RenderFlag_Visible)) continue;
			if (!(item.flags & RenderFlag_CastShadows)) continue;
			if (item.flags & RenderFlag_ForceTransparent) continue;

			uint32_t dynamicOffset = static_cast<uint32_t>(i * m_dynamicAlignment);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipelineLayout, 1, 1,
									&m_perObjectDescriptorSets[m_currentFrame], 1, &dynamicOffset);

			VulkanMeshWrapper &mesh = m_meshes.Get(item.meshID);
			vkCmdDrawIndexed(cmd, mesh.indexCount, 1, mesh.firstIndex, mesh.firstVertex, 0);

			m_stats.drawCalls++;
			m_stats.indexCount += mesh.indexCount;
			m_stats.vertexCount += mesh.vertexCount;
			m_stats.triangleCount += (mesh.indexCount / 3);
		}
	}
	vkCmdEndRendering(cmd);

	shadowBarrier.srcStageMask	= VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	shadowBarrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	shadowBarrier.dstStageMask	= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	shadowBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
	shadowBarrier.oldLayout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	shadowBarrier.newLayout		= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier2(cmd, &shadowDep);

	VkImageMemoryBarrier2 colorBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
	colorBarrier.srcStageMask	  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	colorBarrier.dstStageMask	  = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	colorBarrier.dstAccessMask	  = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	colorBarrier.oldLayout		  = VK_IMAGE_LAYOUT_UNDEFINED;
	colorBarrier.newLayout		  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorBarrier.image			  = m_swapChain->GetImages()[imageIndex];
	colorBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	VkImageMemoryBarrier2 depthBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
	depthBarrier.srcStageMask	  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
	depthBarrier.dstStageMask	  = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
	depthBarrier.dstAccessMask	  = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	depthBarrier.oldLayout		  = VK_IMAGE_LAYOUT_UNDEFINED;
	depthBarrier.newLayout		  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthBarrier.image			  = m_depthTexture.image;
	depthBarrier.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

	VkImageMemoryBarrier2 barriers[] = {colorBarrier, depthBarrier};
	VkDependencyInfo	  mainDep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
	mainDep.imageMemoryBarrierCount = 2;
	mainDep.pImageMemoryBarriers	= barriers;
	vkCmdPipelineBarrier2(cmd, &mainDep);

	VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
	colorAttachment.imageView		 = m_swapChain->GetImageViews()[imageIndex];
	colorAttachment.imageLayout		 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.loadOp			 = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp			 = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.clearValue.color = {{0.1f, 0.1f, 0.1f, 1.0f}};

	VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
	depthAttachment.imageView				= m_depthTexture.imageView;
	depthAttachment.imageLayout				= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.loadOp					= VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp					= VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.clearValue.depthStencil = {1.0f, 0};

	VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
	renderingInfo.renderArea		   = {{0, 0}, m_swapChain->GetExtent()};
	renderingInfo.layerCount		   = 1;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments	   = &colorAttachment;
	renderingInfo.pDepthAttachment	   = &depthAttachment;

	vkCmdBeginRendering(cmd, &renderingInfo);

	if (!m_renderQueue.empty()) {
		VkDeviceSize offsets[]	= {0};
		VkBuffer	 vBuffers[] = {m_vertexBuffer->GetBuffer()};
		vkCmdBindVertexBuffers(cmd, 0, 1, vBuffers, offsets);
		vkCmdBindIndexBuffer(cmd, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
								&m_perPassDescriptorSets[m_currentFrame], 0, nullptr);

		VkExtent2D vkExtent2D = m_swapChain->GetExtent();
		VkViewport viewport	  = {
			  .x	  = 0.0f,
			  .y	  = static_cast<float>(vkExtent2D.height),
			  .width  = static_cast<float>(vkExtent2D.width),
			  .height = -static_cast<float>(vkExtent2D.height),

			  .minDepth = 0.0f,
			  .maxDepth = 1.0f,
		  };
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		VkRect2D scissor = {.offset = {.x = 0, .y = 0}, .extent = vkExtent2D};
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		MaterialID lastMaterialID = INVALID_HANDLE;

		for (size_t i = 0; i < m_renderQueue.size(); i++) {
			const auto &item = m_renderQueue[i];
			if (!(item.flags & RenderFlag_Visible)) continue;

			Material const &mat = m_materials.Get(item.materialID);

			const ShaderType shaderType = m_shaders.Get(mat.GetShaderID()).GetType();

			int passCount = (shaderType == ShaderType::SnowGlobe) ? 2 : 1;

			for (int pass = 0; pass < passCount; ++pass) {
				PipelineDescription desc;
				desc.shaderID		 = mat.GetShaderID();
				desc.colorFormat	 = m_swapChain->GetImageFormat();
				desc.depthFormat	 = m_depthTexture.format;
				desc.wireframe		 = false;
				desc.enableDepthBias = false;
				if (shaderType == ShaderType::SnowGlobe) {
					if (pass == 0) {
						desc.cullMode = VK_CULL_MODE_FRONT_BIT;

						desc.enableDepthTest  = true;
						desc.enableDepthWrite = false;
						desc.enableBlend	  = false;
						desc.compareOp		  = VK_COMPARE_OP_LESS;
					} else {
						desc.cullMode = VK_CULL_MODE_BACK_BIT;

						desc.enableDepthTest  = true;
						desc.enableDepthWrite = false;
						desc.enableBlend	  = true;
						desc.compareOp		  = VK_COMPARE_OP_LESS;
					}
				} else {
					desc.cullMode		  = VK_CULL_MODE_BACK_BIT;
					desc.enableDepthTest  = true;
					desc.enableDepthWrite = true;
					desc.enableBlend	  = false;
					desc.compareOp		  = VK_COMPARE_OP_LESS;
				}
				VulkanPipeline *pipeline = nullptr;
				if (auto it = m_pipelineDescriptions.find(desc); it != m_pipelineDescriptions.end()) {
					pipeline = it->second;
				} else {
					pipeline = new VulkanPipeline();
					if (pipeline->Initialize(ref_device, m_shaders.Get(desc.shaderID), m_pipelineLayout,
											 m_swapChain->GetExtent(), desc) < ERROR_CODE::WARN_START) {
						PE_LOG_FATAL("Vulkan failed to create pipeline.");
						return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;
					}
					m_pipelineDescriptions[desc] = pipeline;
				}

				if (m_currentPipeline != pipeline) {
					m_currentPipeline = pipeline;
					m_currentPipeline->Bind(cmd);
				}

				if (item.materialID != lastMaterialID) {
					vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 2, 1,
											&m_materialDescriptorSets[item.materialID], 0, nullptr);
					lastMaterialID = item.materialID;
				}

				uint32_t dynamicOffset = static_cast<uint32_t>(i * m_dynamicAlignment);
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 1, 1,
										&m_perObjectDescriptorSets[m_currentFrame], 1, &dynamicOffset);

				VulkanMeshWrapper &mesh = m_meshes.Get(item.meshID);
				vkCmdDrawIndexed(cmd, mesh.indexCount, 1, mesh.firstIndex, mesh.firstVertex, 0);

				m_stats.drawCalls++;
				m_stats.indexCount += mesh.indexCount;
				m_stats.vertexCount += mesh.vertexCount;
				m_stats.triangleCount += (mesh.indexCount / 3);
			}
		}
	}

	FlushParticles(cmd);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	vkCmdEndRendering(cmd);

	VkImageMemoryBarrier2 presentBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
	presentBarrier.srcStageMask		= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	presentBarrier.srcAccessMask	= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
	presentBarrier.dstStageMask		= VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	presentBarrier.dstAccessMask	= 0;
	presentBarrier.oldLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	presentBarrier.newLayout		= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	presentBarrier.image			= m_swapChain->GetImages()[imageIndex];
	presentBarrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

	VkDependencyInfo presentDepInfo{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
	presentDepInfo.imageMemoryBarrierCount = 1;
	presentDepInfo.pImageMemoryBarriers	   = &presentBarrier;

	vkCmdPipelineBarrier2(cmd, &presentDepInfo);

	if (vkEndCommandBuffer(cmd) != VK_SUCCESS) return ERROR_CODE::VULKAN_BUFFER_CREATION_FAILED;
	return ERROR_CODE::OK;
}

void VulkanRenderer::UpdateGlobalBuffer(const CBPerPass &data) {
	CBPerPass shadowData = data;

	Math::Vector3 lightDir = Math::Vector3(data.lightDirection.x, data.lightDirection.y, data.lightDirection.z);

	Math::Vector3 lightPos = Math::Normalize(lightDir) * (-110.0f);

	Math::Vector3 target = Math::Vector3Zero;

	auto up = Math::Vector3(0.0f, 1.0f, 0.0f);
	if (Math::Abs(lightDir.y) > 0.99f) {
		up = Math::Vector3(0.0f, 0.0f, 1.0f);
	}

	Math::Matrix4 lightView = Math::Mat4LookAt(lightPos, target, up);
	float		  orthoSize = 100.0f;
	float		  nearPlane = 0.1f;
	float		  farPlane	= 200.0f;

	Math::Matrix4 lightProj		= Math::Mat4Ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
	shadowData.lightSpaceMatrix = lightProj * lightView;
	m_perPassBuffers[m_currentFrame]->Update(&shadowData, sizeof(CBPerPass), 0);
}

void VulkanRenderer::UpdateUniformBuffer(uint32_t currentFrame) {
	VulkanBuffer *objBuffer = m_perObjectBuffers[currentFrame];

	void *rawData = objBuffer->GetMappedData();

	if (!rawData) {
		PE_LOG_ERROR("Object Buffer not mapped! Cannot update uniforms.");
		return;
	}

	char *mappedBytePtr = static_cast<char *>(rawData);

	size_t count = m_renderQueue.size();
	if (count > ref_engineConfig->maxEntityCount) {
		PE_LOG_WARN("Render Queue exceeded buffer capacity! Capping to " +
					std::to_string(ref_engineConfig->maxEntityCount));
		count = ref_engineConfig->maxEntityCount;
	}

	for (size_t i = 0; i < count; i++) {
		const auto &command = m_renderQueue[i];

		CBPerObject objectData{};
		objectData.world = command.worldMatrix;

		size_t offset = i * m_dynamicAlignment;

		memcpy(mappedBytePtr + offset, &objectData, sizeof(CBPerObject));
	}
}

ERROR_CODE VulkanRenderer::UpdateMaterialTexture(MaterialID matID, TextureType typeIdx, TextureID texID) {
	if (matID >= m_materialDescriptorSets.size()) {
		PE_LOG_ERROR("Invalid Material ID for texture update.");
		return ERROR_CODE::VULKAN_MATERIAL_UPDATE_FAILED;
	}

	TextureID validTexID = (texID != INVALID_HANDLE) ? texID : static_cast<TextureID>(typeIdx);

	if (!m_textures.Has(validTexID)) {
		PE_LOG_WARN("Texture ID " + std::to_string(validTexID) + " not found in pool. Using default.");
		validTexID = static_cast<TextureID>(typeIdx);
	}

	VulkanTextureWrapper &tex = m_textures.Get(validTexID);

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView	  = tex.imageView;
	imageInfo.sampler	  = m_globalSamplers[static_cast<uint8_t>(GetMaterial(matID).GetSampler(typeIdx))];

	VkWriteDescriptorSet texWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	texWrite.dstSet			 = m_materialDescriptorSets[matID];
	texWrite.dstBinding		 = static_cast<uint32_t>(typeIdx) + 1;
	texWrite.dstArrayElement = 0;
	texWrite.descriptorType	 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	texWrite.descriptorCount = 1;
	texWrite.pImageInfo		 = &imageInfo;

	vkUpdateDescriptorSets(ref_device->GetVkDevice(), 1, &texWrite, 0, nullptr);

	PE_LOG_INFO("Updated Material " + std::to_string(matID) + " Texture Type " +
				std::to_string(static_cast<uint32_t>(typeIdx)) + " to TextureID " + std::to_string(validTexID));
	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::UpdateMaterial(MaterialID id) {
	if (!m_materials.Has(id)) {
		PE_LOG_ERROR("Attempting to update invalid Material ID");
		return ERROR_CODE::VULKAN_MATERIAL_UPDATE_FAILED;
	}

	if (id >= m_perMaterialBuffers.size()) {
		PE_LOG_ERROR("Material Buffer Index out of bounds");
		return ERROR_CODE::VULKAN_MATERIAL_UPDATE_FAILED;
	}

	const Material &mat	   = m_materials.Get(id);
	VulkanBuffer   *buffer = m_perMaterialBuffers[id];

	const std::vector<uint8_t> &rawData = mat.GetPropertyData();

	if (!rawData.empty()) {
		buffer->WriteToMapped(rawData.data(), rawData.size(), 0);
	} else
		PE_LOG_ERROR("Material has no property data!");

	return ERROR_CODE::OK;
}

// TODO: Not implemented yet.
RenderTargetID VulkanRenderer::CreateRenderTarget(int w, int h, int f) { return 0; }

// TODO: Not implemented yet.
void VulkanRenderer::SetRenderTargets(std::span<const RenderTargetID>(targets), RenderTargetID d) {}

// TODO: Not implemented yet.
void VulkanRenderer::ClearRenderTarget(RenderTargetID id, const float c[4]) {}

// TODO: Not implemented yet.
void VulkanRenderer::ClearDepth(RenderTargetID id) {}

// TODO: Not implemented yet.
TextureID VulkanRenderer::GetRenderTargetTexture(RenderTargetID id) { return 0; }

void VulkanRenderer::WaitIdle() { vkDeviceWaitIdle(ref_device->GetVkDevice()); }

RenderStats VulkanRenderer::GetStats() const { return m_stats; }

TextureID VulkanRenderer::CreateTexture(const std::string &name, const unsigned char *data,
										const TextureParameters &params) {
	VulkanTextureWrapper t;
	t.width		 = static_cast<uint32_t>(params.width);
	t.height	 = static_cast<uint32_t>(params.height);
	t.mipLevels	 = static_cast<uint32_t>(params.mipLevels);
	t.layerCount = params.arrayLayers;
	t.format	 = params.format.vulkanFormat;
	if (t.format == VK_FORMAT_UNDEFINED) t.format = VK_FORMAT_R8G8B8A8_SRGB;
	t.usage	  = params.usage.vulkanUsage;
	t.samples = static_cast<VkSampleCountFlagBits>(params.samples);

	if (params.isCubemap && t.layerCount == 6) {
		t.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	} else {
		t.flags = 0;
	}

	VkDeviceSize imageSize = static_cast<VkDeviceSize>(t.width) * t.height * 4 * t.layerCount;

	VulkanBuffer stagingBuffer;

	ERROR_CODE result = stagingBuffer.Initialize(
		ref_device->GetVkDevice(), ref_device->GetVkPhysicalDevice(), imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (result != ERROR_CODE::OK) {
		PE_LOG_FATAL("Failed to create staging buffer for texture: " + name);
		return INVALID_HANDLE;
	}

	stagingBuffer.Map();
	stagingBuffer.Update(data, imageSize);
	stagingBuffer.Unmap();

	CreateImage(t, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	TransitionImageLayout(t.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, t.layerCount);

	CopyBufferToImage(stagingBuffer.GetBuffer(), t.image, t.width, t.height, t.layerCount);

	TransitionImageLayout(t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						  t.layerCount);

	t.currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	const VkImageViewType viewType = params.isCubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;

	t.imageView = CreateImageView(t.image, t.format, VK_IMAGE_ASPECT_COLOR_BIT, viewType, t.layerCount);

	stagingBuffer.Shutdown();
	return m_textures.Add(std::move(t));
}

MeshID VulkanRenderer::CreateMesh(const std::string &name, const MeshData &meshData) {
	VkDeviceSize vertexDataSize = sizeof(Vertex) * meshData.Vertices.size();

	if (m_currentVertexOffset + vertexDataSize > MAX_VERTEX_BUFFER_SIZE) {
		PE_LOG_FATAL("Global Vertex Buffer is full!");
		return INVALID_HANDLE;
	}

	m_vertexBuffer->UpdateStaged(m_command->GetCommandPool(), ref_device->GetGraphicsQueue(), meshData.Vertices.data(),
								 vertexDataSize, m_currentVertexOffset);

	VkDeviceSize indexDataSize = sizeof(uint32_t) * meshData.Indices.size();

	if (m_currentIndexOffset + indexDataSize > MAX_VERTEX_BUFFER_SIZE) {
		PE_LOG_FATAL("Global Index Buffer is full!");
		return INVALID_HANDLE;
	}

	m_indexBuffer->UpdateStaged(m_command->GetCommandPool(), ref_device->GetGraphicsQueue(), meshData.Indices.data(),
								indexDataSize, m_currentIndexOffset);

	VulkanMeshWrapper m;
	m.vertexBuffer = m_vertexBuffer->GetBuffer();
	m.indexBuffer  = m_indexBuffer->GetBuffer();

	m.vertexCount = static_cast<uint32_t>(meshData.Vertices.size());
	m.indexCount  = static_cast<uint32_t>(meshData.Indices.size());

	m.firstVertex = static_cast<uint32_t>(m_currentVertexOffset / sizeof(Vertex));
	m.firstIndex  = static_cast<uint32_t>(m_currentIndexOffset / sizeof(uint32_t));

	m_currentVertexOffset += vertexDataSize;
	m_currentIndexOffset += indexDataSize;

	return m_meshes.Add(std::move(m));
}

ShaderID VulkanRenderer::CreateShader(const ShaderType type, const std::filesystem::path &vsPath,
									  const std::filesystem::path &psPath) {
	if (VulkanShader s; s.Initialize(ref_device->GetVkDevice(), type, vsPath, psPath) == ERROR_CODE::OK)
		return m_shaders.Add(std::move(s));
	return INVALID_HANDLE;
}

MaterialID VulkanRenderer::CreateMaterial(ShaderID shaderID) {
	auto type = m_shaders.Data()[shaderID].GetType();

	std::array<MaterialPropertyLayout, static_cast<size_t>(MaterialProperty::Count)> matLayout;
	matLayout.fill({0, 0});

	uint32_t bufferSize = 0;

	auto SetLayout = [&](MaterialProperty prop, const size_t offset, const size_t size) {
		matLayout[static_cast<size_t>(prop)] = {static_cast<uint32_t>(offset), static_cast<uint32_t>(size)};
	};

	switch (type) {
		case ShaderType::Lit: {
			bufferSize = sizeof(CBMaterial_Lit);
			SetLayout(MaterialProperty::Color, offsetof(CBMaterial_Lit, diffuseColor), sizeof(Math::Vector4));
			SetLayout(MaterialProperty::SpecularColor, offsetof(CBMaterial_Lit, specularColor), sizeof(Math::Vector3));
			SetLayout(MaterialProperty::SpecularPower, offsetof(CBMaterial_Lit, specularPower), sizeof(float));
			SetLayout(MaterialProperty::Tiling, offsetof(CBMaterial_Lit, tiling), sizeof(Math::Vector2));
			SetLayout(MaterialProperty::Offset, offsetof(CBMaterial_Lit, offset), sizeof(Math::Vector2));
		} break;

		case ShaderType::Unlit: {
			bufferSize = sizeof(CBMaterial_Unlit);
			SetLayout(MaterialProperty::Color, offsetof(CBMaterial_Unlit, color), sizeof(Math::Vector4));
			SetLayout(MaterialProperty::Tiling, offsetof(CBMaterial_Unlit, tiling), sizeof(Math::Vector2));
			SetLayout(MaterialProperty::Offset, offsetof(CBMaterial_Unlit, offset), sizeof(Math::Vector2));
		} break;

		case ShaderType::PBR: {
			bufferSize = sizeof(CBMaterial_PBR);
			SetLayout(MaterialProperty::Color, offsetof(CBMaterial_PBR, albedoColor), sizeof(Math::Vector4));
			SetLayout(MaterialProperty::Roughness, offsetof(CBMaterial_PBR, roughness), sizeof(float));
			SetLayout(MaterialProperty::Metallic, offsetof(CBMaterial_PBR, metallic), sizeof(float));
			SetLayout(MaterialProperty::NormalStrength, offsetof(CBMaterial_PBR, normalStrength), sizeof(float));
			SetLayout(MaterialProperty::OcclusionStrength, offsetof(CBMaterial_PBR, occlusionStrength), sizeof(float));
			SetLayout(MaterialProperty::EmissiveColor, offsetof(CBMaterial_PBR, emissiveColor), sizeof(Math::Vector3));
			SetLayout(MaterialProperty::EmissiveIntensity, offsetof(CBMaterial_PBR, emissiveIntensity), sizeof(float));
			SetLayout(MaterialProperty::Tiling, offsetof(CBMaterial_PBR, tiling), sizeof(Math::Vector2));
			SetLayout(MaterialProperty::Offset, offsetof(CBMaterial_PBR, offset), sizeof(Math::Vector2));
		} break;

		case ShaderType::Terrain: {
			bufferSize = sizeof(CBMaterial_Terrain);
			SetLayout(MaterialProperty::LayerTiling, offsetof(CBMaterial_Terrain, layerTiling), sizeof(Math::Vector4));
			SetLayout(MaterialProperty::BlendDistance, offsetof(CBMaterial_Terrain, blendDistance), sizeof(float));
			SetLayout(MaterialProperty::BlendFalloff, offsetof(CBMaterial_Terrain, blendFalloff), sizeof(float));
		} break;

		case ShaderType::UI: {
			bufferSize = sizeof(CBMaterial_UI);
			SetLayout(MaterialProperty::Color, offsetof(CBMaterial_UI, color), sizeof(Math::Vector4));
			SetLayout(MaterialProperty::Opacity, offsetof(CBMaterial_UI, opacity), sizeof(float));
			SetLayout(MaterialProperty::BorderThickness, offsetof(CBMaterial_UI, borderThickness), sizeof(float));
			SetLayout(MaterialProperty::Softness, offsetof(CBMaterial_UI, softness), sizeof(float));
			SetLayout(MaterialProperty::BorderColor, offsetof(CBMaterial_UI, borderColor), sizeof(Math::Vector4));
		} break;

		case ShaderType::SnowGlobe: {
			bufferSize = sizeof(CBMaterial_Lit);
			SetLayout(MaterialProperty::Color, offsetof(CBMaterial_Lit, diffuseColor), sizeof(Math::Vector4));
			SetLayout(MaterialProperty::SpecularColor, offsetof(CBMaterial_Lit, specularColor), sizeof(Math::Vector3));
			SetLayout(MaterialProperty::SpecularPower, offsetof(CBMaterial_Lit, specularPower), sizeof(float));
			SetLayout(MaterialProperty::Tiling, offsetof(CBMaterial_Lit, tiling), sizeof(Math::Vector2));
			SetLayout(MaterialProperty::Offset, offsetof(CBMaterial_Lit, offset), sizeof(Math::Vector2));
		} break;
		default: PE_LOG_ERROR("Not implemented!"); break;
	}

	const uint32_t matID = m_materials.Add(std::move(Material()));
	Material	  &mat	 = GetMaterial(matID);

	mat.Initialize(this, matID, shaderID, bufferSize, matLayout);
	m_perMaterialBuffers.emplace_back(new VulkanBuffer());
	VulkanBuffer *matBuffer = m_perMaterialBuffers.back();
	VkDeviceSize  safeSize	= bufferSize > 0 ? bufferSize : 64;

	ERROR_CODE result = matBuffer->Initialize(
		ref_device->GetVkDevice(), ref_device->GetVkPhysicalDevice(), safeSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (result < ERROR_CODE::WARN_START) {
		Utilities::SafeShutdown(matBuffer);
		PE_LOG_FATAL("Vulkan can't created material!");
		return INVALID_HANDLE;
	}

	matBuffer->Map();

	VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	allocInfo.descriptorPool	 = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts		 = &m_perMaterialSetLayout;

	VkDescriptorSet matSet;
	if (vkAllocateDescriptorSets(ref_device->GetVkDevice(), &allocInfo, &matSet) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to allocate Material Descriptor Set!");
		return INVALID_HANDLE;
	}

	m_materialDescriptorSets.push_back(matSet);

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = matBuffer->GetBuffer();
	bufferInfo.offset = 0;
	bufferInfo.range  = VK_WHOLE_SIZE;

	VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
	write.dstSet		  = matSet;
	write.dstBinding	  = 0;
	write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo	  = &bufferInfo;

	vkUpdateDescriptorSets(ref_device->GetVkDevice(), 1, &write, 0, nullptr);

	const auto &materialTextures = mat.GetTextures();

	for (size_t i = 0; i < materialTextures.size(); ++i) {
		// Determine which texture to use.
		// If the material has a valid texture, use it. Otherwise, fallback to default textures.
		TextureID texID = materialTextures[i];
		if (texID == INVALID_HANDLE) {
			// First 12 textures are default textures based on TextureType enum.
			texID = Assets::AssetManager::RequestDefaultTexture(static_cast<TextureType>(i));
		}

		UpdateMaterialTexture(matID, static_cast<TextureType>(i), texID);
	}

	auto HasProp = [&](MaterialProperty prop) { return matLayout[static_cast<size_t>(prop)].size > 0; };

	// Default UVs
	if (HasProp(MaterialProperty::Tiling)) mat.SetProperty(MaterialProperty::Tiling, Math::Vector2(1.0f, 1.0f));
	if (HasProp(MaterialProperty::Offset)) mat.SetProperty(MaterialProperty::Offset, Math::Vector2(0.0f, 0.0f));

	// Default Colors (White)
	if (HasProp(MaterialProperty::Color))
		mat.SetProperty(MaterialProperty::Color, Math::Vector4(1.0f, 1.0f, 1.0f, 1.0f));

	// Type Specific Defaults
	if (type == ShaderType::Lit) {
		mat.SetProperty(MaterialProperty::SpecularColor, Math::Vector3(0.5f, 0.5f, 0.5f));
		mat.SetProperty(MaterialProperty::SpecularPower, 32.0f);
	} else if (type == ShaderType::PBR) {
		mat.SetProperty(MaterialProperty::Roughness, 0.5f);
		mat.SetProperty(MaterialProperty::Metallic, 0.0f);
		mat.SetProperty(MaterialProperty::NormalStrength, 1.0f);
		mat.SetProperty(MaterialProperty::OcclusionStrength, 1.0f);
		mat.SetProperty(MaterialProperty::EmissiveIntensity, 1.0f);
	} else if (type == ShaderType::Terrain) {
		mat.SetProperty(MaterialProperty::LayerTiling, Math::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		mat.SetProperty(MaterialProperty::BlendDistance, 10.0f);
		mat.SetProperty(MaterialProperty::BlendFalloff, 1.0f);
	} else if (type == ShaderType::UI) {
		mat.SetProperty(MaterialProperty::Opacity, 1.0f);
	}

	const auto &propData = mat.GetPropertyData();
	if (!propData.empty()) {
		matBuffer->WriteToMapped(propData.data(), propData.size(), 0);
	}

	return matID;
}

ERROR_CODE VulkanRenderer::CreateGlobalTextureSamplers() {
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(ref_device->GetVkPhysicalDevice(), &properties);

	VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	info.borderColor			 = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	info.unnormalizedCoordinates = VK_FALSE;
	info.compareEnable			 = VK_FALSE;
	info.mipmapMode				 = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	auto Create = [&](SamplerType type, VkFilter minMag, VkSamplerAddressMode addr, bool anisotropy = false) {
		info.minFilter		  = minMag;
		info.magFilter		  = minMag;
		info.addressModeU	  = addr;
		info.addressModeV	  = addr;
		info.addressModeW	  = addr;
		info.anisotropyEnable = anisotropy ? VK_TRUE : VK_FALSE;
		info.maxAnisotropy	  = anisotropy ? properties.limits.maxSamplerAnisotropy : 1.0f;

		info.mipmapMode =
			(minMag == VK_FILTER_NEAREST) ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;

		return vkCreateSampler(ref_device->GetVkDevice(), &info, nullptr, &m_globalSamplers[static_cast<size_t>(type)]);
	};

	// 1. Linear Repeat
	if (Create(SamplerType::LinearRepeat, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to create LinearRepeat Sampler!");
		return ERROR_CODE::VULKAN_SAMPLER_CREATION_FAILED;
	}

	// 2. Linear Clamp
	if (Create(SamplerType::LinearClamp, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to create LinearClamp Sampler!");
		return ERROR_CODE::VULKAN_SAMPLER_CREATION_FAILED;
	}

	// 3. Point Repeat
	if (Create(SamplerType::PointRepeat, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to create PointRepeat Sampler!");
		return ERROR_CODE::VULKAN_SAMPLER_CREATION_FAILED;
	}

	// 4. Point Clamp
	if (Create(SamplerType::PointClamp, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to create PointClamp Sampler!");
		return ERROR_CODE::VULKAN_SAMPLER_CREATION_FAILED;
	}

	// 5. Anisotropic
	if (Create(SamplerType::Anisotropic, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, true) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to create Anisotropic Sampler!");
		return ERROR_CODE::VULKAN_SAMPLER_CREATION_FAILED;
	}

	// --- 6. Shadow PCF ---
	info.anisotropyEnable		 = VK_FALSE;
	info.maxAnisotropy			 = 1.0f;
	info.mipLodBias				 = 0.0f;
	info.minLod					 = 0.0f;
	info.maxLod					 = 1.0f;
	info.unnormalizedCoordinates = VK_FALSE;

	info.magFilter	   = VK_FILTER_LINEAR;
	info.minFilter	   = VK_FILTER_LINEAR;
	info.mipmapMode	   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	info.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	info.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	info.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	info.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	info.compareEnable = VK_TRUE;
	info.compareOp	   = VK_COMPARE_OP_LESS;

	if (vkCreateSampler(ref_device->GetVkDevice(), &info, nullptr,
						&m_globalSamplers[static_cast<size_t>(SamplerType::ShadowPCF)]) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create sampler: ShadowPCF");
		return ERROR_CODE::VULKAN_SAMPLER_CREATION_FAILED;
	}

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding perPassBindings[2];

	perPassBindings[0].binding			  = 0;
	perPassBindings[0].descriptorType	  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	perPassBindings[0].descriptorCount	  = 1;
	perPassBindings[0].stageFlags		  = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	perPassBindings[0].pImmutableSamplers = nullptr;

	perPassBindings[1].binding			  = 1;
	perPassBindings[1].descriptorType	  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	perPassBindings[1].descriptorCount	  = 1;
	perPassBindings[1].stageFlags		  = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
	perPassBindings[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo perPassInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	perPassInfo.bindingCount = 2;
	perPassInfo.pBindings	 = perPassBindings;

	if (vkCreateDescriptorSetLayout(ref_device->GetVkDevice(), &perPassInfo, nullptr, &m_perPassSetLayout) !=
		VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create descriptor set layout!");
		return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;
	}

	VkDescriptorSetLayoutBinding perObjectLayoutBinding{
		.binding			= 0,
		.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		.descriptorCount	= 1,
		.stageFlags			= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = nullptr};

	const VkDescriptorSetLayoutCreateInfo perObjectInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
														.bindingCount = 1,
														.pBindings	  = &perObjectLayoutBinding};

	if (vkCreateDescriptorSetLayout(ref_device->GetVkDevice(), &perObjectInfo, nullptr, &m_perObjectSetLayout) !=
		VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create descriptor set layout!");
		return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;
	}

	std::vector<VkDescriptorSetLayoutBinding> perMatLayoutBindings;

	VkDescriptorSetLayoutBinding propBinding{};
	propBinding.binding			= 0;
	propBinding.descriptorType	= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	propBinding.descriptorCount = 1;
	propBinding.stageFlags		= VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	perMatLayoutBindings.push_back(propBinding);

	constexpr size_t textureCount = static_cast<size_t>(TextureType::Count);

	for (uint32_t i = 0; i < textureCount; i++) {
		VkDescriptorSetLayoutBinding texBinding{};
		texBinding.binding			  = i + 1;
		texBinding.descriptorType	  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		texBinding.descriptorCount	  = 1;
		texBinding.pImmutableSamplers = nullptr;
		texBinding.stageFlags		  = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		perMatLayoutBindings.push_back(texBinding);
	}

	VkDescriptorSetLayoutCreateInfo materialInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	materialInfo.bindingCount = static_cast<uint32_t>(perMatLayoutBindings.size());
	materialInfo.pBindings	  = perMatLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(ref_device->GetVkDevice(), &materialInfo, nullptr, &m_perMaterialSetLayout) !=
		VK_SUCCESS) {
		PE_LOG_FATAL("Failed to create Material Layout");
		return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;
	}

	m_descriptorSetLayouts = {m_perPassSetLayout, m_perObjectSetLayout, m_perMaterialSetLayout};

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateStandardPipelineLayout() {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType				  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount		  = static_cast<uint32_t>(m_descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts			  = m_descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(ref_device->GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) !=
		VK_SUCCESS) {
		PE_LOG_FATAL("failed to create pipeline layout!");
		return ERROR_CODE::VULKAN_SHADER_PIPELINE_FAILED;
	}
	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateDepthBuffer() {
	if (m_depthTexture.image != VK_NULL_HANDLE)
		vkDestroyImage(ref_device->GetVkDevice(), m_depthTexture.image, nullptr);
	if (m_depthTexture.imageView != VK_NULL_HANDLE)
		vkDestroyImageView(ref_device->GetVkDevice(), m_depthTexture.imageView, nullptr);
	if (m_depthTexture.memory != VK_NULL_HANDLE)
		vkFreeMemory(ref_device->GetVkDevice(), m_depthTexture.memory, nullptr);

	const VkFormat depthFormat = FindDepthFormat();
	auto [width, height]	   = m_swapChain->GetExtent();

	m_depthTexture.width	  = width;
	m_depthTexture.height	  = height;
	m_depthTexture.depth	  = 1;
	m_depthTexture.format	  = depthFormat;
	m_depthTexture.mipLevels  = 1;
	m_depthTexture.layerCount = 1;
	m_depthTexture.samples	  = VK_SAMPLE_COUNT_1_BIT;
	m_depthTexture.usage	  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	m_depthTexture.flags	  = 0;

	CreateImage(m_depthTexture, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_depthTexture.imageView = CreateImageView(m_depthTexture.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT,
											   VK_IMAGE_VIEW_TYPE_2D, m_depthTexture.layerCount);
	return ERROR_CODE::OK;
}

VkFormat VulkanRenderer::FindDepthFormat() const {
	return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
							   VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat VulkanRenderer::FindSupportedFormat(const std::vector<VkFormat> &candidates, const VkImageTiling tiling,
											 const VkFormatFeatureFlags features) const {
	for (const VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(ref_device->GetVkPhysicalDevice(), format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) return format;
		if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return format;
	}

	PE_LOG_FATAL("Vulkan failed to find supported format for request!");
	return {};
}

ERROR_CODE VulkanRenderer::CreateShadowResources() {
	m_shadowMap.texture.width	   = m_shadowMap.dim;
	m_shadowMap.texture.height	   = m_shadowMap.dim;
	m_shadowMap.texture.format	   = VK_FORMAT_D32_SFLOAT;
	m_shadowMap.texture.usage	   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	m_shadowMap.texture.samples	   = VK_SAMPLE_COUNT_1_BIT;
	m_shadowMap.texture.mipLevels  = 1;
	m_shadowMap.texture.layerCount = 1;

	CreateImage(m_shadowMap.texture, VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	m_shadowMap.texture.imageView = CreateImageView(m_shadowMap.texture.image, m_shadowMap.texture.format,
													VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D, 1);

	m_shadowMap.sampler = m_globalSamplers[static_cast<size_t>(SamplerType::ShadowPCF)];

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateShadowPipeline() {
	VulkanShader const &shadowShader = m_shaders.Get(Assets::AssetManager::DefaultShadowShaderID);

	const VkDescriptorSetLayout layouts[] = {m_perPassSetLayout, m_perObjectSetLayout};
	VkPipelineLayoutCreateInfo	pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts	  = layouts;

	vkCreatePipelineLayout(ref_device->GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_shadowPipelineLayout);

	std::vector<VkVertexInputBindingDescription>   bindings	   = {Vertex::GetBindingDescription()};
	auto										   attribArray = Vertex::GetAttributeDescriptions();
	std::vector<VkVertexInputAttributeDescription> attribs(attribArray.begin(), attribArray.end());

	PipelineDescription desc;
	desc.colorFormat	  = VK_FORMAT_UNDEFINED;
	desc.depthFormat	  = VK_FORMAT_D32_SFLOAT;
	desc.cullMode		  = VK_CULL_MODE_BACK_BIT;
	desc.enableDepthWrite = true;
	desc.enableDepthTest  = true;
	desc.enableBlend	  = false;
	desc.enableDepthBias  = true;

	m_shadowPipeline = new VulkanPipeline();
	m_shadowPipeline->Initialize(ref_device, shadowShader, m_shadowPipelineLayout, {m_shadowMap.dim, m_shadowMap.dim},
								 desc, bindings, attribs);

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateParticleResources() {
	VkDevice device = ref_device->GetVkDevice();

	// 1. Create Instance Buffers (Double Buffered for Frame-in-Flight)
	m_particleInstanceBuffers.resize(ref_renderConfig->maxFramesInFlight);
	VkDeviceSize bufferSize = ref_renderConfig->maxParticlesPerFrame * sizeof(GPUInstanceData);

	for (int i = 0; i < ref_renderConfig->maxFramesInFlight; i++) {
		m_particleInstanceBuffers[i] = new VulkanBuffer();
		// VERTEX_BUFFER_BIT is critical because we bind this as an Instanced Vertex Buffer
		m_particleInstanceBuffers[i]->Initialize(
			device, ref_device->GetVkPhysicalDevice(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		m_particleInstanceBuffers[i]->Map();
	}

	// 2. Create Descriptor Layout (Set 1: Single Texture Sampler)
	// Set 0 is "PerPass" (Camera/Global), which we reuse from standard pipeline.
	VkDescriptorSetLayoutBinding samplerBinding{};
	samplerBinding.binding		   = 0;
	samplerBinding.descriptorCount = 1;
	samplerBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerBinding.stageFlags	   = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings	= &samplerBinding;

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_particleSetLayout) != VK_SUCCESS)
		return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;

	// 3. Create Pipeline Layout (Set 0 + Set 1)
	VkDescriptorSetLayout layouts[] = {m_perPassSetLayout, m_particleSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts	  = layouts;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_particlePipelineLayout) != VK_SUCCESS)
		return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateParticlePipeline() {
	// 1. Load Shaders (Ensure these files exist in your assets folder!)
	VulkanShader &particleShader = m_shaders.Get(Assets::AssetManager::DefaultParticleShaderID);

	// 2. Define Vertex Input (The Critical Part)
	const std::vector bindings({Vertex::GetBindingDescription(), GPUInstanceData::GetBindingDescription()});

	std::vector<VkVertexInputAttributeDescription> attribs;
	const auto vertexParticleAttribs = Vertex::GetAttributeDescriptionForParticles();
	const auto instanceDataAttribs	 = GPUInstanceData::GetAttributeDescription();

	for (auto &vertDesc : vertexParticleAttribs) attribs.push_back(vertDesc);
	for (auto &instDesc : instanceDataAttribs) attribs.push_back(instDesc);

	// 3. Pipeline Config
	PipelineDescription desc;
	desc.enableBlend	  = true;	// Particles need blending!
	desc.enableDepthWrite = false;	// Soft particles: Don't write depth, but DO test depth
	desc.enableDepthTest  = true;
	desc.enableDepthBias  = false;
	desc.cullMode		  = VK_CULL_MODE_NONE;	// View-aligned quads can be finicky, safer to disable cull
	desc.colorFormat	  = m_swapChain->GetImageFormat();
	desc.depthFormat	  = m_depthTexture.format;
	desc.wireframe		  = false;

	m_particlePipeline = new VulkanPipeline();
	// Use the Custom Overload you wrote
	m_particlePipeline->Initialize(ref_device, particleShader, m_particlePipelineLayout, m_swapChain->GetExtent(), desc,
								   bindings, attribs);

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateVertexBuffer() {
	m_vertexBuffer			 = new VulkanBuffer();
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	auto result = m_vertexBuffer->Initialize(ref_device->GetVkDevice(), ref_device->GetVkPhysicalDevice(),
											 MAX_VERTEX_BUFFER_SIZE,  // 50MB sabit boyut
											 usage, VK_SHARING_MODE_EXCLUSIVE,
											 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  // GPU tarafında allocate ediyoruz
	);

	if (result < ERROR_CODE::WARN_START) {
		Utilities::SafeShutdown(m_vertexBuffer);
		PE_LOG_FATAL("Failed to create Global Vertex Buffer!");
		return result;
	}

	m_currentVertexOffset = 0;

	return result;
}

ERROR_CODE VulkanRenderer::CreateIndexBuffer() {
	m_indexBuffer			 = new VulkanBuffer();
	VkBufferUsageFlags usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	auto result = m_indexBuffer->Initialize(ref_device->GetVkDevice(), ref_device->GetVkPhysicalDevice(),
											MAX_VERTEX_BUFFER_SIZE,	 // 50MB sabit boyut
											usage, VK_SHARING_MODE_EXCLUSIVE,
											VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT	 // GPU tarafında allocate ediyoruz
	);

	if (result < ERROR_CODE::WARN_START) {
		Utilities::SafeShutdown(m_indexBuffer);
		PE_LOG_FATAL("Failed to create Global Vertex Buffer!");
		return result;
	}

	m_currentIndexOffset = 0;

	return result;
}

ERROR_CODE VulkanRenderer::CreateUniformBuffers(const uint32_t maxModelCount) {
	// Resize vectors to match frames in flight (defined in your Config or Constants)
	// Assuming ref_renderConfig.maxFramesInFlight is e.g., 2
	m_perPassBuffers.resize(ref_renderConfig->maxFramesInFlight);
	m_perObjectBuffers.resize(ref_renderConfig->maxFramesInFlight);

	VkDeviceSize globalSize = sizeof(CBPerPass);

	// Calculate dynamic alignment for Object Buffer
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(ref_device->GetVkPhysicalDevice(), &properties);
	VkDeviceSize minAlignment = properties.limits.minUniformBufferOffsetAlignment;

	m_dynamicAlignment = sizeof(CBPerObject);

	// Round up to the next multiple of minAlignment
	if (minAlignment > 0) {
		m_dynamicAlignment = (m_dynamicAlignment + minAlignment - 1) & ~(minAlignment - 1);
	}

	// Allocate enough size for MAX_OBJECTS (e.g., 10,000) * Aligned Size
	VkDeviceSize objectBufferSize = m_dynamicAlignment * maxModelCount;

	for (int i = 0; i < ref_renderConfig->maxFramesInFlight; i++) {
		// 1. Create Global Buffer (Set 0)
		m_perPassBuffers[i] = new VulkanBuffer();
		ERROR_CODE result	= m_perPassBuffers[i]->Initialize(
			  ref_device->GetVkDevice(), ref_device->GetVkPhysicalDevice(), globalSize,
			  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			  VK_SHARING_MODE_EXCLUSIVE,												  // Standard mode
			  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  // Coherent = no need to Flush()
		  );
		if (result < ERROR_CODE::WARN_START) {
			Utilities::SafeShutdown(m_perPassBuffers[i]);
			PE_LOG_FATAL("Vulkan failed to create per pass buffer!");
			return result;
		}

		m_perPassBuffers[i]->Map();

		// 2. Create Object Buffer (Set 1)
		m_perObjectBuffers[i] = new VulkanBuffer();
		result				  = m_perObjectBuffers[i]->Initialize(
			   ref_device->GetVkDevice(), ref_device->GetVkPhysicalDevice(), objectBufferSize,
			   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
			   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (result < ERROR_CODE::WARN_START) {
			Utilities::SafeShutdown(m_perObjectBuffers[i]);
			PE_LOG_FATAL("Vulkan failed to create per object buffer!");
			return result;
		}

		m_perObjectBuffers[i]->Map();
	}

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateDescriptorPool() {
	uint32_t maxFrames = static_cast<uint32_t>(ref_renderConfig->maxFramesInFlight);
	// Arbitrary limit for materials (e.g., 1000 materials)
	uint32_t maxMaterials = ref_renderConfig->maxMaterialCount;

	std::vector<VkDescriptorPoolSize> poolSizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxFrames + maxMaterials},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, maxFrames},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (maxMaterials * 8) + maxFrames}};

	VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes	   = poolSizes.data();
	// Max Sets = Frames (Set 0) + Frames (Set 1) + Materials (Set 2)
	poolInfo.maxSets = (maxFrames * 2) + maxMaterials + 50;

	if (vkCreateDescriptorPool(ref_device->GetVkDevice(), &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS) {
		PE_LOG_FATAL("Vulkan failed to create Descriptor Pool!");
		return ERROR_CODE::VULKAN_DEVICE_CREATION_FAILED;
	}

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateDescriptorSets() {
	const uint32_t frames = ref_renderConfig->maxFramesInFlight;

	// Resize vectors to hold the sets
	m_perPassDescriptorSets.resize(frames);
	m_perObjectDescriptorSets.resize(frames);

	VkDevice device = ref_device->GetVkDevice();

	// Loop through each frame-in-flight to allocate its specific sets
	for (uint32_t i = 0; i < frames; i++) {
		// =============================================================
		// 1. ALLOCATE SET 0 (Global / Per Pass)
		// =============================================================
		VkDescriptorSetAllocateInfo passAllocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		passAllocInfo.descriptorPool	 = m_descriptorPool;
		passAllocInfo.descriptorSetCount = 1;
		passAllocInfo.pSetLayouts		 = &m_perPassSetLayout;

		if (vkAllocateDescriptorSets(device, &passAllocInfo, &m_perPassDescriptorSets[i]) != VK_SUCCESS) {
			PE_LOG_FATAL("Failed to allocate Per-Pass Descriptor Set!");
			return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;
		}

		// 2. WRITE TO SET 0 (Binding 0 + Binding 1)
		std::array<VkWriteDescriptorSet, 2> passWrites{};

		// Binding 0: Uniform Buffer
		VkDescriptorBufferInfo perPassBufferInfo{};
		perPassBufferInfo.buffer = m_perPassBuffers[i]->GetBuffer();
		perPassBufferInfo.offset = 0;
		perPassBufferInfo.range	 = sizeof(CBPerPass);

		passWrites[0].sType			  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		passWrites[0].dstSet		  = m_perPassDescriptorSets[i];
		passWrites[0].dstBinding	  = 0;
		passWrites[0].descriptorCount = 1;
		passWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		passWrites[0].pBufferInfo	  = &perPassBufferInfo;

		// Binding 1: Shadow Map Texture (YENİ)
		VkDescriptorImageInfo shadowImageInfo{};
		shadowImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	 // Shader okuyacak
		shadowImageInfo.imageView	= m_shadowMap.texture.imageView;
		shadowImageInfo.sampler		= m_globalSamplers[static_cast<size_t>(SamplerType::ShadowPCF)];

		passWrites[1].sType			  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		passWrites[1].dstSet		  = m_perPassDescriptorSets[i];
		passWrites[1].dstBinding	  = 1;	// Binding 1
		passWrites[1].descriptorCount = 1;
		passWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		passWrites[1].pImageInfo	  = &shadowImageInfo;

		// Update both bindings at once
		vkUpdateDescriptorSets(device, 2, passWrites.data(), 0, nullptr);

		// =============================================================
		// 3. ALLOCATE SET 1 (Per Object - Dynamic)
		// =============================================================
		VkDescriptorSetAllocateInfo objAllocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
		objAllocInfo.descriptorPool		= m_descriptorPool;
		objAllocInfo.descriptorSetCount = 1;
		objAllocInfo.pSetLayouts		= &m_perObjectSetLayout;

		if (vkAllocateDescriptorSets(device, &objAllocInfo, &m_perObjectDescriptorSets[i]) != VK_SUCCESS) {
			PE_LOG_FATAL("Failed to allocate Per-Object Descriptor Set!");
			return ERROR_CODE::VULKAN_PIPELINE_CREATION_FAILED;
		}

		// =============================================================
		// 4. WRITE TO SET 1
		// =============================================================
		// Note: For Dynamic Buffers, we point to the BASE of the buffer (offset 0).
		// The 'range' is the size of ONE instance of the struct (aligned is safer, but sizeof works usually).
		// The dynamic offset applied in vkCmdBindDescriptorSets will shift this window.
		VkDescriptorBufferInfo perObjBufferInfo{};
		perObjBufferInfo.buffer = m_perObjectBuffers[i]->GetBuffer();
		perObjBufferInfo.offset = 0;
		perObjBufferInfo.range	= m_dynamicAlignment;

		VkWriteDescriptorSet objWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		objWrite.dstSet			 = m_perObjectDescriptorSets[i];
		objWrite.dstBinding		 = 0;
		objWrite.dstArrayElement = 0;
		// CRITICAL: Must match layout (DYNAMIC)
		objWrite.descriptorType	 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		objWrite.descriptorCount = 1;
		objWrite.pBufferInfo	 = &perObjBufferInfo;

		vkUpdateDescriptorSets(device, 1, &objWrite, 0, nullptr);
	}

	return ERROR_CODE::OK;
}

ERROR_CODE VulkanRenderer::CreateSyncObjects(const int maxFramesInFlight) {
	m_imageAvailableSemaphores.resize(maxFramesInFlight);
	size_t swapChainImageCount = m_swapChain->GetImageViews().size();
	m_renderFinishedSemaphores.resize(swapChainImageCount);
	m_inFlightFences.resize(maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT};

	for (int i = 0; i < maxFramesInFlight; ++i) {
		if (vkCreateSemaphore(ref_device->GetVkDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) ||
			vkCreateFence(ref_device->GetVkDevice(), &fenceInfo, nullptr, &m_inFlightFences[i])) {
			PE_LOG_FATAL("Vulkan failed to create frame sync objects!");
			return ERROR_CODE::VULKAN_SYNCOBJECTS_CREATION_FAILED;
		}
	}

	for (size_t i = 0; i < swapChainImageCount; ++i) {
		if (vkCreateSemaphore(ref_device->GetVkDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i])) {
			PE_LOG_FATAL("Vulkan failed to create render finished semaphores!");
			return ERROR_CODE::VULKAN_SYNCOBJECTS_CREATION_FAILED;
		}
	}
	return ERROR_CODE::OK;
}

void VulkanRenderer::CreateImage(VulkanTextureWrapper &tW, const VkImageTiling tiling,
								 const VkMemoryPropertyFlags properties) {
	VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	imageInfo.imageType		= VK_IMAGE_TYPE_2D;
	imageInfo.extent.width	= tW.width;
	imageInfo.extent.height = tW.height;
	imageInfo.extent.depth	= tW.depth;
	imageInfo.mipLevels		= tW.mipLevels;
	imageInfo.arrayLayers	= tW.layerCount;
	imageInfo.format		= tW.format;
	imageInfo.tiling		= tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage			= tW.usage;
	imageInfo.samples		= tW.samples;
	imageInfo.sharingMode	= VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags			= tW.flags;

	if (vkCreateImage(ref_device->GetVkDevice(), &imageInfo, nullptr, &tW.image) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(ref_device->GetVkDevice(), tW.image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocInfo.allocationSize  = memRequirements.size;
	allocInfo.memoryTypeIndex = ref_device->FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(ref_device->GetVkDevice(), &allocInfo, nullptr, &tW.memory) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to allocate image memory!");
	}

	vkBindImageMemory(ref_device->GetVkDevice(), tW.image, tW.memory, 0);
}

void VulkanRenderer::TransitionImageLayout(const VkImage image, const VkImageLayout oldLayout,
										   const VkImageLayout newLayout, const uint32_t layerCount) {
	const VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
	barrier.oldLayout						= oldLayout;
	barrier.newLayout						= newLayout;
	barrier.srcQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex				= VK_QUEUE_FAMILY_IGNORED;
	barrier.image							= image;
	barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel	= 0;
	barrier.subresourceRange.levelCount		= 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount		= layerCount;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage			  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage	  = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
			   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage			  = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage	  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		PE_LOG_WARN("Unsupported layout transition!");
		EndSingleTimeCommands(commandBuffer);
		return;
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::CopyBufferToImage(const VkBuffer buffer, const VkImage image, const uint32_t width,
									   const uint32_t height, const uint32_t layerCount) {
	const VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	std::vector<VkBufferImageCopy> regions;
	regions.reserve(layerCount);

	const VkDeviceSize faceSize = static_cast<VkDeviceSize>(width) * height * 4;

	for (uint32_t i = 0; i < layerCount; ++i) {
		VkBufferImageCopy region{};
		region.bufferOffset					   = i * faceSize;
		region.bufferRowLength				   = 0;
		region.bufferImageHeight			   = 0;
		region.imageSubresource.aspectMask	   = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel	   = 0;
		region.imageSubresource.baseArrayLayer = i;
		region.imageSubresource.layerCount	   = 1;
		region.imageOffset					   = {0, 0, 0};
		region.imageExtent					   = {width, height, 1};
		regions.push_back(region);
	}
	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						   static_cast<uint32_t>(regions.size()), regions.data());

	EndSingleTimeCommands(commandBuffer);
}

VkImageView VulkanRenderer::CreateImageView(const VkImage image, const VkFormat format,
											const VkImageAspectFlags aspectFlags, const VkImageViewType viewType,
											const uint32_t layerCount) const {
	VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	viewInfo.image							 = image;
	viewInfo.viewType						 = viewType;
	viewInfo.format							 = format;
	viewInfo.subresourceRange.aspectMask	 = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel	 = 0;
	viewInfo.subresourceRange.levelCount	 = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount	 = layerCount;

	VkImageView imageView;
	if (vkCreateImageView(ref_device->GetVkDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		PE_LOG_FATAL("Failed to create image view!");
	}
	return imageView;
}

void VulkanRenderer::RecreateSwapchain(int width, int height) {
	WaitIdle();
	m_swapChain->Recreate(width, height);
	CreateDepthBuffer();

	size_t newImageCount = m_swapChain->GetImageViews().size();
	if (newImageCount != m_renderFinishedSemaphores.size()) {
		for (auto sem : m_renderFinishedSemaphores) vkDestroySemaphore(ref_device->GetVkDevice(), sem, nullptr);

		m_renderFinishedSemaphores.resize(newImageCount);
		VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		for (size_t i = 0; i < newImageCount; ++i) {
			vkCreateSemaphore(ref_device->GetVkDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
		}
	}
}

VkCommandBuffer VulkanRenderer::BeginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	allocInfo.level				 = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool		 = m_command->GetCommandPool();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(ref_device->GetVkDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void VulkanRenderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &commandBuffer;

	vkQueueSubmit(ref_device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(ref_device->GetGraphicsQueue());

	vkFreeCommandBuffers(ref_device->GetVkDevice(), m_command->GetCommandPool(), 1, &commandBuffer);
}
}  // namespace PE::Graphics::Vulkan