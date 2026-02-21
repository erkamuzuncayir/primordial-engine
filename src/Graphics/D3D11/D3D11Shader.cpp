#ifdef PE_D3D11
#include <Graphics/D3D11/D3D11Shader.h>
#include <d3dcompiler.h>

#include "Assets/AssetManager.h"
#include "Graphics/D3D11/D3D11Utilities.h"
#include "Utilities/IOUtilities.h"

namespace PE::Graphics::D3D11 {
D3D11Shader::D3D11Shader() {}

D3D11Shader::~D3D11Shader() { Shutdown(); }

D3D11Shader::D3D11Shader(D3D11Shader &&other) noexcept {
	m_vertexShader	 = other.m_vertexShader;
	m_pixelShader	 = other.m_pixelShader;
	m_layout		 = other.m_layout;
	m_materialBuffer = other.m_materialBuffer;
	m_type			 = other.m_type;

	other.m_vertexShader   = nullptr;
	other.m_pixelShader	   = nullptr;
	other.m_layout		   = nullptr;
	other.m_materialBuffer = nullptr;
}

D3D11Shader &D3D11Shader::operator=(D3D11Shader &&other) noexcept {
	if (this != &other) {
		Shutdown();

		m_vertexShader	 = other.m_vertexShader;
		m_pixelShader	 = other.m_pixelShader;
		m_layout		 = other.m_layout;
		m_materialBuffer = other.m_materialBuffer;
		m_type			 = other.m_type;

		other.m_vertexShader   = nullptr;
		other.m_pixelShader	   = nullptr;
		other.m_layout		   = nullptr;
		other.m_materialBuffer = nullptr;
	}
	return *this;
}

ERROR_CODE D3D11Shader::Initialize(ID3D11Device *device, const ShaderType type, const std::filesystem::path &vsPath,
								   const std::filesystem::path &psPath) {
	PE_CHECK_STATE_INIT(m_state, "This D3D11 shader is already initialized.");
	m_state = SystemState::Initializing;

	m_type	= type;
	auto hr = S_OK;

	std::vector<char> vsBlob;
	ERROR_CODE		  result;
	PE_CHECK(result, Utilities::IOUtilities::ReadBinaryFile(vsPath.string(), vsBlob));

	hr = device->CreateVertexShader(vsBlob.data(), vsBlob.size(), nullptr, &m_vertexShader);
	if (FAILED(hr)) {
		PE_LOG_ERROR("Failed to create Vertex Shader.");
		return ERROR_CODE::SHADER_CANT_CREATED;
	}

	std::vector<char> psBlob;
	PE_CHECK(result, Utilities::IOUtilities::ReadBinaryFile(psPath, psBlob));

	hr = device->CreatePixelShader(psBlob.data(), psBlob.size(), nullptr, &m_pixelShader);
	if (FAILED(hr)) {
		PE_LOG_ERROR("Failed to create Pixel Shader.");
		return ERROR_CODE::SHADER_CANT_CREATED;
	}

	D3D11_INPUT_ELEMENT_DESC layoutDesc[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}};

	hr = device->CreateInputLayout(layoutDesc, 4, vsBlob.data(), vsBlob.size(), &m_layout);
	if (FAILED(hr)) {
		PE_LOG_ERROR("Failed to create Input Layout.");
		return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;
	}

	D3D11_BUFFER_DESC matDesc = {};
	matDesc.Usage			  = D3D11_USAGE_DYNAMIC;
	switch (type) {
		case ShaderType::Unlit: matDesc.ByteWidth = sizeof(CBMaterial_Unlit); break;
		case ShaderType::Lit: matDesc.ByteWidth = sizeof(CBMaterial_Lit); break;
		case ShaderType::PBR: matDesc.ByteWidth = sizeof(CBMaterial_PBR); break;
		case ShaderType::Terrain: matDesc.ByteWidth = sizeof(CBMaterial_Terrain); break;
		case ShaderType::UI: matDesc.ByteWidth = sizeof(CBMaterial_UI); break;
		default: PE_LOG_ERROR("Not implemented!"); break;
	}
	matDesc.BindFlags	   = D3D11_BIND_CONSTANT_BUFFER;
	matDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	hr = device->CreateBuffer(&matDesc, nullptr, &m_materialBuffer);
	if (FAILED(hr)) {
		PE_LOG_ERROR("Failed to create Material Buffer.");
		return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;
	}

	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

void D3D11Shader::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return;

	m_state = SystemState::ShuttingDown;
	SafeRelease(m_materialBuffer);
	SafeRelease(m_layout);
	SafeRelease(m_pixelShader);
	SafeRelease(m_vertexShader);

	m_state = SystemState::Uninitialized;
}

void D3D11Shader::Bind(ID3D11DeviceContext *context) {
	context->IASetInputLayout(m_layout);
	context->VSSetShader(m_vertexShader, nullptr, 0);
	context->PSSetShader(m_pixelShader, nullptr, 0);

	if (m_materialBuffer) {
		context->PSSetConstantBuffers(2, 1, &m_materialBuffer);
	}
}

void D3D11Shader::UpdateMaterialBuffer(const ShaderType type, ID3D11DeviceContext *context, const void *data) {
	if (!m_materialBuffer) return;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	if (SUCCEEDED(context->Map(m_materialBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
		size_t sizeOfData = 0;
		switch (type) {
			case ShaderType::Unlit: sizeOfData = sizeof(CBMaterial_Unlit); break;
			case ShaderType::Lit: sizeOfData = sizeof(CBMaterial_Lit); break;
			case ShaderType::PBR: sizeOfData = sizeof(CBMaterial_PBR); break;
			case ShaderType::Terrain: sizeOfData = sizeof(CBMaterial_Terrain); break;
			case ShaderType::UI: sizeOfData = sizeof(CBMaterial_UI); break;
			default: PE_LOG_ERROR("Not implemented!"); break;
		}
		memcpy(mappedResource.pData, data, sizeOfData);
		context->Unmap(m_materialBuffer, 0);
	}
}
}  // namespace PE::Graphics::D3D11
#endif