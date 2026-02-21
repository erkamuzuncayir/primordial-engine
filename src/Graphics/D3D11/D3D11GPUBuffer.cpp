#ifdef PE_D3D11
#include "Graphics/D3D11/D3D11GPUBuffer.h"

#include "Graphics/D3D11/D3D11Utilities.h"

namespace PE::Graphics::D3D11 {
D3D11GPUBuffer::D3D11GPUBuffer() {}

D3D11GPUBuffer::~D3D11GPUBuffer() { Shutdown(); }

ERROR_CODE D3D11GPUBuffer::Initialize(ID3D11Device *device, uint32_t byteWidth, uint32_t bindFlags, uint32_t usage,
									  uint32_t cpuAccessFlags) {
	PE_CHECK_STATE_INIT(m_state, "This D3D11 buffer is already initialized.");
	m_state = SystemState::Initializing;

	D3D11_BUFFER_DESC vbd	= {};
	vbd.ByteWidth			= byteWidth;
	vbd.Usage				= static_cast<D3D11_USAGE>(usage);
	vbd.BindFlags			= bindFlags;
	vbd.CPUAccessFlags		= cpuAccessFlags;
	vbd.MiscFlags			= 0;
	vbd.StructureByteStride = 0;

	HRESULT result = LOG_HR_RESULT(device->CreateBuffer(&vbd, nullptr, &m_buffer), Utilities::LogLevel::Fatal,
								   "HRESULT failed while creating vertex buffer.");
	if (FAILED(result)) return ERROR_CODE::DX11_BUFFER_CREATION_FAILED;

	m_size	= byteWidth;
	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

ERROR_CODE D3D11GPUBuffer::Initialize(ID3D11Device *device, const void *data, uint32_t byteWidth, uint32_t bindFlags,
									  uint32_t usage) {
	PE_CHECK_STATE_INIT(m_state, "This D3D11 buffer is already initialized.");
	m_state = SystemState::Initializing;

	D3D11_BUFFER_DESC ibd = {};
	ibd.ByteWidth		  = byteWidth;
	ibd.Usage			  = static_cast<D3D11_USAGE>(usage);
	ibd.BindFlags		  = bindFlags;
	ibd.CPUAccessFlags	  = 0;
	ibd.MiscFlags		  = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem				= data;
	initData.SysMemPitch			= 0;
	initData.SysMemSlicePitch		= 0;

	HRESULT result = LOG_HR_RESULT(device->CreateBuffer(&ibd, &initData, &m_buffer), Utilities::LogLevel::Fatal,
								   "HRESULT failed while creating index buffer.");
	if (FAILED(result)) return ERROR_CODE::DX11_SHADER_PIPELINE_FAILED;

	m_size	= byteWidth;
	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

ERROR_CODE D3D11GPUBuffer::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;

	m_state = SystemState::ShuttingDown;
	SafeRelease(m_buffer);

	m_state = SystemState::Uninitialized;
	return ERROR_CODE::OK;
}

void D3D11GPUBuffer::Update(ID3D11DeviceContext *context, const void *data) {
	if (!m_buffer) return;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	// TODO: Split for immutable vertex and index buffers otherwise it can't map.
	const HRESULT hr = LOG_HR_RESULT(context->Map(m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource),
									 Utilities::LogLevel::Fatal, "HRESULT failed while mapping buffer.");

	if (SUCCEEDED(hr)) {
		memcpy(mappedResource.pData, data, m_size);
		context->Unmap(m_buffer, 0);
	}
}
}  // namespace PE::Graphics::D3D11
#endif