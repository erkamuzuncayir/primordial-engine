#pragma once

#include <filesystem>
#include <span>

#include "Common/Common.h"
#include "Components/ParticleEmitter.h"
#include "Core/EngineConfig.h"
#include "GLFW/glfw3.h"
#include "Material.h"
#include "RenderTypes.h"

namespace PE::Assets {
class AssetManager;
}

struct GLFWwindow;

namespace PE::Graphics {
struct RenderStats {
	uint32_t vertexCount   = 0;
	uint32_t indexCount	   = 0;
	uint32_t triangleCount = 0;
	uint32_t drawCalls	   = 0;
};

/**
 * @brief An abstract interface for a graphics renderer.
 * This allows the RenderSystem to be API-agnostic.
 */
class IRenderer {
	friend class Assets::AssetManager;

public:
	virtual ~IRenderer() = default;

	virtual ERROR_CODE Initialize(GLFWwindow *windowHandle, const Core::EngineConfig &config) = 0;
	virtual ERROR_CODE CreateDefaultResources()												  = 0;
	virtual ERROR_CODE Shutdown()															  = 0;
	virtual ERROR_CODE OnResize(const RenderConfig &config)									  = 0;

	virtual ERROR_CODE InitGUI()	 = 0;
	virtual void	   NewFrameGUI() = 0;
	virtual void	   ShutdownGUI() = 0;

	virtual void Submit(const RenderCommand &command)															  = 0;
	virtual void SubmitParticles(TextureID texture, const std::vector<Graphics::Components::Particle> &particles) = 0;
	virtual void Flush()																						  = 0;

	virtual void	   UpdateGlobalBuffer(const CBPerPass &data)									 = 0;
	virtual ERROR_CODE UpdateMaterialTexture(MaterialID matID, TextureType typeIdx, TextureID texID) = 0;
	virtual ERROR_CODE UpdateMaterial(uint32_t matID)												 = 0;

	virtual RenderTargetID CreateRenderTarget(int width, int height, int format)								  = 0;
	virtual void		   SetRenderTargets(std::span<const RenderTargetID> targets, RenderTargetID depthStencil) = 0;
	virtual void		   ClearRenderTarget(RenderTargetID id, const float color[4])							  = 0;
	virtual void		   ClearDepth(RenderTargetID id)														  = 0;
	[[nodiscard]] virtual TextureID GetRenderTargetTexture(RenderTargetID id)									  = 0;

	[[nodiscard]] virtual Material &GetMaterial(MaterialID id) = 0;

	[[nodiscard]] virtual RenderStats GetStats() const = 0;

private:
	virtual TextureID  CreateTexture(const std::string &name, const unsigned char *data,
									 const TextureParameters &params)				 = 0;
	virtual MeshID	   CreateMesh(const std::string &name, const MeshData &meshData) = 0;
	virtual ShaderID   CreateShader(ShaderType type, const std::filesystem::path &vsPath,
									const std::filesystem::path &psPath)			 = 0;
	virtual MaterialID CreateMaterial(ShaderID shaderID)							 = 0;
};
}  // namespace PE::Graphics