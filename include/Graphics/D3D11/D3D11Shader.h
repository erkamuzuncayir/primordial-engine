#pragma once

#ifdef PE_D3D11
#include <d3d11.h>

#include <filesystem>

#include "Common/Common.h"
#include "Graphics/RenderTypes.h"

namespace PE::Graphics::D3D11 {
class D3D11Shader {
public:
	D3D11Shader();
	~D3D11Shader();

	D3D11Shader(const D3D11Shader &)			= delete;
	D3D11Shader &operator=(const D3D11Shader &) = delete;

	D3D11Shader(D3D11Shader &&other) noexcept;
	D3D11Shader &operator=(D3D11Shader &&other) noexcept;

	ERROR_CODE Initialize(ID3D11Device *device, ShaderType type, const std::filesystem::path &vsPath,
						  const std::filesystem::path &psPath);
	void	   Shutdown();

	// Binds the shader and layout the pipeline
	void Bind(ID3D11DeviceContext *context);

	void UpdateMaterialBuffer(ShaderType type, ID3D11DeviceContext *context, const void *data);

	ShaderType GetType() const { return m_type; }

private:
	SystemState			m_state		   = SystemState::Uninitialized;
	ShaderType			m_type		   = ShaderType::Count;
	ID3D11VertexShader *m_vertexShader = nullptr;
	ID3D11PixelShader  *m_pixelShader  = nullptr;
	ID3D11InputLayout  *m_layout	   = nullptr;

	// Slot 2: Material Buffer (Optional, depends on shader)
	ID3D11Buffer *m_materialBuffer = nullptr;
};
}  // namespace PE::Graphics::D3D11
#endif