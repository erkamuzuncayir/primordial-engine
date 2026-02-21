#pragma once

#include <span>
#include <vector>

#include "Core/EngineConfig.h"
#include "Graphics/IRenderer.h"
#include "Graphics/Material.h"
#include "Graphics/RenderConfig.h"
#include "Graphics/ResourcePool.h"
#include "Graphics/Vulkan/VulkanBuffer.h"
#include "Graphics/Vulkan/VulkanCommand.h"
#include "Graphics/Vulkan/VulkanDevice.h"
#include "Graphics/Vulkan/VulkanShader.h"
#include "Graphics/Vulkan/VulkanSwapchain.h"
#include "Graphics/Vulkan/VulkanTypes.h"
#include "VulkanPipeline.h"

namespace PE::Graphics::Vulkan {
constexpr VkDeviceSize MAX_VERTEX_BUFFER_SIZE = 1024 * 1024 * 256;

class VulkanRenderer : public IRenderer {
public:
	VulkanRenderer()								  = default;
	VulkanRenderer(const VulkanRenderer &)			  = delete;
	VulkanRenderer &operator=(const VulkanRenderer &) = delete;
	VulkanRenderer(VulkanRenderer &&)				  = delete;
	VulkanRenderer &operator=(VulkanRenderer &&)	  = delete;
	~VulkanRenderer() override						  = default;

	ERROR_CODE Initialize(GLFWwindow *windowHandle, const Core::EngineConfig &config) override;
	ERROR_CODE CreateDefaultResources() override;
	ERROR_CODE Shutdown() override;
	ERROR_CODE OnResize(const RenderConfig &config) override;

	ERROR_CODE InitGUI() override;
	void	   NewFrameGUI() override;
	void	   ShutdownGUI() override;

	void Submit(const RenderCommand &command) override;
	void SubmitParticles(TextureID texture, const std::vector<Graphics::Components::Particle> &particles) override;
	void Flush() override;
	void FlushParticles(VkCommandBuffer cmd);
	ERROR_CODE RecordCommandBuffer(uint32_t imageIndex, VkCommandBuffer cmd);
	void	   UpdateGlobalBuffer(const CBPerPass &data) override;
	void	   UpdateUniformBuffer(uint32_t currentFrame);
	ERROR_CODE UpdateMaterialTexture(MaterialID matID, TextureType typeIdx, TextureID texID) override;
	// TODO: Combine this and UpdateMaterialBuffer in D3D11Shader and convert it into a single API.
	ERROR_CODE UpdateMaterial(MaterialID id) override;

	RenderTargetID CreateRenderTarget(int width, int height, int format) override;
	void		   SetRenderTargets(std::span<const RenderTargetID>(targets), RenderTargetID depthStencil) override;
	void		   ClearRenderTarget(RenderTargetID id, const float color[4]) override;
	void		   ClearDepth(RenderTargetID id) override;
	TextureID	   GetRenderTargetTexture(RenderTargetID id) override;
	[[nodiscard]] VkImageView GetDepthBufferImageView() const { return m_depthTexture.imageView; }

	Material &GetMaterial(MaterialID id) override { return m_materials.Get(id); }
	void	  WaitIdle();

	[[nodiscard]] RenderStats GetStats() const override;

private:
	TextureID  CreateTexture(const std::string &name, const unsigned char *data,
							 const TextureParameters &params) override;
	MeshID	   CreateMesh(const std::string &name, const MeshData &meshData) override;
	ShaderID   CreateShader(ShaderType type, const std::filesystem::path &vsPath,
							const std::filesystem::path &psPath) override;
	MaterialID CreateMaterial(ShaderID shaderID) override;
	ERROR_CODE CreateGlobalTextureSamplers();

	ERROR_CODE			   CreateDescriptorSetLayout();
	ERROR_CODE			   CreateStandardPipelineLayout();
	ERROR_CODE			   CreateDepthBuffer();
	[[nodiscard]] VkFormat FindDepthFormat() const;
	[[nodiscard]] VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling,
											   VkFormatFeatureFlags features) const;
	ERROR_CODE			   CreateShadowResources();
	ERROR_CODE			   CreateShadowPipeline();
	ERROR_CODE			   CreateParticleResources();
	ERROR_CODE			   CreateParticlePipeline();
	ERROR_CODE			   CreateVertexBuffer();
	ERROR_CODE			   CreateIndexBuffer();
	ERROR_CODE			   CreateUniformBuffers(uint32_t maxModelCount);
	ERROR_CODE			   CreateDescriptorPool();
	ERROR_CODE			   CreateDescriptorSets();
	ERROR_CODE			   CreateSyncObjects(int maxFramesInFlight);
	void CreateImage(VulkanTextureWrapper &tW, VkImageTiling tiling, VkMemoryPropertyFlags properties);
	void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);
	VkImageView		CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags,
									VkImageViewType viewType, uint32_t layerCount) const;
	void			RecreateSwapchain(int width, int height);
	VkCommandBuffer BeginSingleTimeCommands();
	void			EndSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
	GLFWwindow				 *ref_window = nullptr;
	const Core::EngineConfig *ref_engineConfig;
	const RenderConfig		 *ref_renderConfig;
	VulkanDevice			 *ref_device = nullptr;

	SystemState		 m_state			   = SystemState::Uninitialized;
	VkDeviceSize	 m_currentVertexOffset = 0;
	VkDeviceSize	 m_currentIndexOffset  = 0;
	RenderStats		 m_stats;
	VulkanSwapchain *m_swapChain		= nullptr;
	VulkanCommand	*m_command			= nullptr;
	VulkanPipeline	*m_currentPipeline	= nullptr;
	VulkanPipeline	*m_particlePipeline = nullptr;
	VulkanPipeline	*m_shadowPipeline	= nullptr;

	std::vector<VkDescriptorSetLayout>			   m_descriptorSetLayouts;
	std::vector<VkDescriptorSet>				   m_perPassDescriptorSets;
	std::vector<VkDescriptorSet>				   m_perObjectDescriptorSets;
	std::vector<VkDescriptorSet>				   m_materialDescriptorSets;
	std::unordered_map<TextureID, VkDescriptorSet> m_particleTextureSets;

	VkDescriptorSetLayout m_perPassSetLayout	 = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_perObjectSetLayout	 = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_perMaterialSetLayout = VK_NULL_HANDLE;
	VkDescriptorSetLayout m_particleSetLayout	 = VK_NULL_HANDLE;

	VkPipelineLayout m_pipelineLayout		  = VK_NULL_HANDLE;
	VkPipelineLayout m_particlePipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout m_shadowPipelineLayout	  = VK_NULL_HANDLE;

	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

	VkDeviceSize m_dynamicAlignment = 0;

	std::vector<VkSemaphore> m_imageAvailableSemaphores;
	std::vector<VkSemaphore> m_renderFinishedSemaphores;
	std::vector<VkFence>	 m_inFlightFences;

	// IMGUI
	VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;

	std::unordered_map<PipelineDescription, VulkanPipeline *, PipelineDescription::PipelineDescriptionHash>
		m_pipelineDescriptions;

	std::vector<RenderCommand>	m_renderQueue;
	std::vector<VulkanBuffer *> m_perPassBuffers;
	std::vector<VulkanBuffer *> m_perObjectBuffers;
	std::vector<VulkanBuffer *> m_perMaterialBuffers;
	std::vector<VulkanBuffer *> m_particleInstanceBuffers;
	VulkanBuffer			   *m_vertexBuffer = nullptr;
	VulkanBuffer			   *m_indexBuffer  = nullptr;

	VulkanTextureWrapper	   m_depthTexture;
	ShadowMapResources		   m_shadowMap;
	std::vector<ParticleBatch> m_particleBatches;

	std::array<VkSampler, static_cast<size_t>(SamplerType::Count)> m_globalSamplers;
	ResourcePool<VulkanRenderTargetWrapper>						   m_renderTargets;
	ResourcePool<VulkanShader>									   m_shaders;
	ResourcePool<VulkanTextureWrapper>							   m_textures;
	ResourcePool<VulkanMeshWrapper>								   m_meshes;
	ResourcePool<Material>										   m_materials;

	uint32_t			m_currentFrame		 = 0;
	std::pair<int, int> m_lastWidthAndHeight = {0, 0};
};
}  // namespace PE::Graphics::Vulkan