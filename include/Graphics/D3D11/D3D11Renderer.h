#pragma once

#ifdef PE_D3D11
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "Core/EngineConfig.h"
#include "D3D11GPUBuffer.h"
#include "D3D11Shader.h"
#include "D3D11Types.h"
#include "Graphics/IRenderer.h"
#include "Graphics/ResourcePool.h"

namespace PE::Graphics::D3D11 {
class D3D11Renderer : public IRenderer {
public:
	D3D11Renderer();
	D3D11Renderer(const D3D11Renderer &)			= delete;
	D3D11Renderer &operator=(const D3D11Renderer &) = delete;
	D3D11Renderer(D3D11Renderer &&)					= delete;
	D3D11Renderer &operator=(D3D11Renderer &&)		= delete;
	~D3D11Renderer() override;

	ERROR_CODE Initialize(GLFWwindow *window, const Core::EngineConfig &config) override;

	ERROR_CODE CreateDefaultResources() override {
		PE_LOG_FATAL("Not implemented");
		return ERROR_CODE::NOT_IMPLEMENTED;
	}

	ERROR_CODE Shutdown() override;
	ERROR_CODE OnResize(const RenderConfig &config) override;

	ERROR_CODE InitGUI() override {
		PE_LOG_FATAL("Not implemented");
		return ERROR_CODE::NOT_IMPLEMENTED;
	}

	void NewFrameGUI() override { PE_LOG_FATAL("Not implemented"); }
	void ShutdownGUI() override { PE_LOG_FATAL("Not implemented"); }

	void Submit(const RenderCommand &command) override;

	void SubmitParticles(TextureID texture, const std::vector<Components::Particle> &particles) override {
		PE_LOG_FATAL("Not implemented");
	}

	void Flush() override;

	RenderTargetID CreateRenderTarget(int width, int height, int format) override;
	void		   SetRenderTargets(std::span<const RenderTargetID> targets, RenderTargetID depthStencil) override;
	void		   ClearRenderTarget(RenderTargetID id, const float color[4]) override;
	void		   ClearDepth(RenderTargetID id) override;
	TextureID	   GetRenderTargetTexture(RenderTargetID id) override;

	Material &GetMaterial(const MaterialID id) override { return m_materials.Get(id); }

	void	   UpdateGlobalBuffer(const CBPerPass &data) override;
	ERROR_CODE UpdateMaterialTexture(MaterialID matID, TextureType typeIdx, TextureID texID) override;
	// TODO: Combine this and the UpdateMaterialBuffer in D3d11Shader and convert it to a single API.
	ERROR_CODE UpdateMaterial(uint32_t matID) override {
		PE_LOG_FATAL("Not implemented");
		return ERROR_CODE::NOT_IMPLEMENTED;
	}

private:
	ERROR_CODE InitializeD3D11(const RenderConfig &config, HWND hwnd);
	ERROR_CODE GetDedicatedGPUAdapter(IDXGIAdapter *&adapter);
	ERROR_CODE EnumerateAdapters();
	ERROR_CODE CreateConstantBuffers();
	void	   BeginPass(RenderPass pass);
	void	   CreateGBuffer(int width, int height);
	void	   CreateSamplers();

	TextureID  CreateTexture(const std::string &name, const unsigned char *data,
							 const TextureParameters &params) override;
	MeshID	   CreateMesh(const std::string &name, const MeshData &meshData) override;
	ShaderID   CreateShader(ShaderType type, const std::filesystem::path &vsPath,
							const std::filesystem::path &psPath) override;
	MaterialID CreateMaterial(ShaderID shaderID) override;

public:
	RenderStats GetStats() const override {
		PE_LOG_ERROR("Not implemented");
		return RenderStats();
	}

private:
	const Core::EngineConfig *ref_engineConfig	 = nullptr;
	const RenderConfig		 *ref_renderConfig	 = nullptr;
	SystemState				  m_state			 = SystemState::Uninitialized;
	ID3D11Device			 *m_device			 = nullptr;
	ID3D11DeviceContext		 *m_context			 = nullptr;
	IDXGISwapChain			 *m_swapChain		 = nullptr;
	ID3D11RenderTargetView	 *m_renderTargetView = nullptr;
	ID3D11DepthStencilView	 *m_depthStencilView = nullptr;
	// ID3D11Texture2D* m_depthStencilBuffer = nullptr;

	D3D11_VIEWPORT m_viewport;

	RenderTargetID m_gbufferIDs[3];

	ResourcePool<D3D11Shader>			   m_shaders;
	ResourcePool<D3D11TextureWrapper>	   m_textures;
	ResourcePool<D3D11MeshWrapper>		   m_meshes;
	ResourcePool<Material>				   m_materials;
	ResourcePool<D3D11RenderTargetWrapper> m_renderTargets;

	std::vector<RenderCommand> m_renderQueue;

	D3D11GPUBuffer *m_perObjectBuffer = nullptr;
	D3D11GPUBuffer *m_perFrameBuffer  = nullptr;

	ID3D11SamplerState *m_linearSampler = nullptr;
	ID3D11SamplerState *m_pointSampler	= nullptr;
};
}  // namespace PE::Graphics::D3D11
#endif