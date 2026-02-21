#pragma once

#ifdef PE_D3D11
#include <d3d11.h>

#include <cstdint>

#include "Common/Common.h"

namespace PE::Graphics::D3D11 {
class D3D11GPUBuffer {
public:
	D3D11GPUBuffer();
	D3D11GPUBuffer(const D3D11GPUBuffer &)			  = delete;
	D3D11GPUBuffer &operator=(const D3D11GPUBuffer &) = delete;
	D3D11GPUBuffer(D3D11GPUBuffer &&)				  = delete;
	D3D11GPUBuffer &operator=(D3D11GPUBuffer &&)	  = delete;
	~D3D11GPUBuffer();

	// TODO: Unify this initializations with enum or split class
	// Initialization for Dynamic Buffers (Constant Buffers)
	ERROR_CODE Initialize(ID3D11Device *device, uint32_t byteWidth, uint32_t bindFlags, uint32_t usage,
						  uint32_t cpuAccessFlags);
	// Initialization for Static Buffers (Vertex/Index)
	ERROR_CODE Initialize(ID3D11Device *device, const void *data, uint32_t byteWidth, uint32_t bindFlags,
						  uint32_t usage = D3D11_USAGE_IMMUTABLE);
	ERROR_CODE Shutdown();

	[[nodiscard]] uint32_t GetSize() const { return m_size; }

	void Update(ID3D11DeviceContext *context, const void *data);

	[[nodiscard]] ID3D11Buffer *GetBuffer() const { return m_buffer; }

private:
	SystemState	  m_state  = SystemState::Uninitialized;
	ID3D11Buffer *m_buffer = nullptr;
	uint32_t	  m_size   = 0;
};
}  // namespace PE::Graphics::D3D11
#endif