#ifdef PE_D3D11
#include <GLFW/glfw3.h>	 // For GLFW
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "Graphics/D3D11/D3D11GPUBuffer.h"
#include "Graphics/D3D11/D3D11Renderer.h"
#include "Graphics/D3D11/D3D11Utilities.h"
#include "Graphics/RenderConfig.h"
#include "Utilities/MemoryUtilities.h"

namespace PE::Graphics::D3D11 {
D3D11Renderer::D3D11Renderer() {}

D3D11Renderer::~D3D11Renderer() { D3D11Renderer::Shutdown(); }

ERROR_CODE D3D11Renderer::Initialize(GLFWwindow *window, const Core::EngineConfig &config) {
	PE_CHECK_STATE_INIT(m_state, "D3D11 renderer is already initialized.");
	m_state = SystemState::Initializing;

	HWND hwnd		 = glfwGetWin32Window(window);
	ref_engineConfig = &config;
	ref_renderConfig = &config.renderConfig;
	ERROR_CODE result;
	PE_CHECK(result, InitializeD3D11(*ref_renderConfig, hwnd));
	PE_CHECK(result, CreateConstantBuffers());
	CreateSamplers();
	if (ref_renderConfig->renderPath == RenderPathType::Deferred)
		CreateGBuffer(ref_renderConfig->width, ref_renderConfig->height);

	PE_LOG_INFO("D3D11 Renderer Initialized Successfully.");
	m_state = SystemState::Running;
	return ERROR_CODE::OK;
}

ERROR_CODE D3D11Renderer::Shutdown() {
	if (m_state == SystemState::Uninitialized || m_state == SystemState::ShuttingDown) return ERROR_CODE::OK;

	m_state = SystemState::ShuttingDown;

	auto result = ERROR_CODE::OK;
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain) {
		if (FAILED(LOG_HR_RESULT(m_swapChain->SetFullscreenState(false, nullptr), Utilities::LogLevel::Fatal,
								 "HRESULT Failed while setting full screen state!")))
			result = ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;
	}

	for (auto &mesh : m_meshes.Data()) {
		SafeRelease(mesh.vb);
		SafeRelease(mesh.ib);
	}
	m_meshes.Clear();

	for (auto &tex : m_textures.Data()) {
		SafeRelease(tex.texture);
		SafeRelease(tex.srv);
	}
	m_textures.Clear();

	for (auto &shader : m_shaders.Data()) shader.Shutdown();
	m_shaders.Clear();

	for (D3D11RenderTargetWrapper &rt : m_renderTargets.Data()) {
		SafeRelease(rt.rtv);
		SafeRelease(rt.srv);
		SafeRelease(rt.dsv);
	}
	m_renderTargets.Clear();

	Utilities::SafeShutdown(m_perFrameBuffer);
	Utilities::SafeShutdown(m_perObjectBuffer);
	SafeRelease(m_linearSampler);
	SafeRelease(m_pointSampler);
	SafeRelease(m_depthStencilView);
	SafeRelease(m_renderTargetView);
	SafeRelease(m_context);
	SafeRelease(m_swapChain);
	SafeRelease(m_device);

	m_state = SystemState::Uninitialized;
	return result;
}

ERROR_CODE D3D11Renderer::OnResize(const RenderConfig &config) {
	if (!m_device || !m_context || !m_swapChain) {
		PE_LOG_FATAL("D3D11Renderer::OnResize called with null pointers.");
		return ERROR_CODE::NULL_POINTER;
	}

	m_context->OMSetRenderTargets(0, nullptr, nullptr);
	SafeRelease(m_renderTargetView);
	SafeRelease(m_depthStencilView);

	// Resize the swapchain and recreate the render target view.
	auto hResult =
		LOG_HR_RESULT(m_swapChain->ResizeBuffers(1, config.width, config.height, DXGI_FORMAT_R8G8B8A8_UNORM, 0),
					  Utilities::LogLevel::Fatal, "HRESULT failed while resizing swap chain!");
	if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

	ID3D11Texture2D *backBuffer;
	hResult =
		LOG_HR_RESULT(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&backBuffer)),
					  Utilities::LogLevel::Fatal, "HRESULT failed while getting back buffer!");
	if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

	hResult = LOG_HR_RESULT(m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView),
							Utilities::LogLevel::Fatal, "HRESULT failed while creating render target view!");
	if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

	// GetBuffer increase the ref count of backBuffer, so we need to release it.
	SafeRelease(backBuffer);

	// Create the depth/stencil buffer and view.
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width	   = config.width;
	depthStencilDesc.Height	   = config.height;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format	   = DXGI_FORMAT_D24_UNORM_S8_UINT;
	if (config.enable4xMSAA) {
		depthStencilDesc.SampleDesc.Count	= config.msaaCount;
		depthStencilDesc.SampleDesc.Quality = config.maxMsaaQuality - 1;
	} else {
		depthStencilDesc.SampleDesc.Count	= 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}
	depthStencilDesc.Usage			= D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags		= D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags		= 0;

	ID3D11Texture2D *depthStencilBuffer;
	hResult = LOG_HR_RESULT(m_device->CreateTexture2D(&depthStencilDesc, nullptr, &depthStencilBuffer),
							Utilities::LogLevel::Fatal, "HRESULT failed while start to creating depth stencil buffer!");
	if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

	hResult = LOG_HR_RESULT(m_device->CreateDepthStencilView(depthStencilBuffer, nullptr, &m_depthStencilView),
							Utilities::LogLevel::Fatal, "HRESULT failed while creating depth stencil buffer!");
	if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width	  = config.width;
	viewport.Height	  = config.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	m_context->RSSetViewports(1, &viewport);

	if (config.renderPath == RenderPathType::Forward) CreateGBuffer(config.width, config.height);

	return ERROR_CODE::OK;
}

void D3D11Renderer::Submit(const RenderCommand &command) { m_renderQueue.push_back(command); }

void D3D11Renderer::Flush() {
	if (m_renderQueue.empty()) return;

	std::sort(m_renderQueue.begin(), m_renderQueue.end(),
			  [](const RenderCommand &a, const RenderCommand &b) { return a.key < b.key; });

	RenderPass currentPass	= static_cast<RenderPass>(255);
	ShaderID   lastShader	= INVALID_HANDLE;
	MaterialID lastMaterial = INVALID_HANDLE;

	auto *shaders	= m_shaders.Data().data();
	auto *materials = m_materials.Data().data();
	auto *meshes	= m_meshes.Data().data();
	auto *textures	= m_textures.Data().data();

	for (const auto &cmd : m_renderQueue) {
		if (const auto cmdPass = static_cast<RenderPass>(cmd.key >> 60); cmdPass != currentPass) {
			BeginPass(cmdPass);
			currentPass	 = cmdPass;
			lastShader	 = INVALID_HANDLE;
			lastMaterial = INVALID_HANDLE;
		}

		Material &mat = materials[cmd.materialID];

		if (mat.GetShaderID() != lastShader) {
			shaders[mat.GetShaderID()].Bind(m_context);
			lastShader	 = mat.GetShaderID();
			lastMaterial = INVALID_HANDLE;
		}

		if (cmd.materialID != lastMaterial) {
			Material &material = materials[cmd.materialID];

			const auto &textureTypes = material.GetTextures();

			for (size_t i = 0; i < textureTypes.size(); ++i) {
				ID3D11ShaderResourceView *srv = nullptr;

				if (const TextureID texID = textureTypes[i]; texID != INVALID_HANDLE) srv = textures[texID].srv;

				m_context->PSSetShaderResources(static_cast<uint32_t>(i), 1, &srv);
			}

			auto &shader = shaders[lastShader];
			shader.UpdateMaterialBuffer(shader.GetType(), m_context, material.GetPropertyData().data());
			lastMaterial = cmd.materialID;
		}

		m_perObjectBuffer->Update(m_context, &cmd.worldMatrix);
		ID3D11Buffer *cb = m_perObjectBuffer->GetBuffer();
		m_context->VSSetConstantBuffers(1, 1, &cb);

		D3D11MeshWrapper &mesh	 = meshes[cmd.meshID];
		UINT			  stride = mesh.stride;
		UINT			  offset = 0;
		m_context->IASetVertexBuffers(0, 1, &(mesh.vb), &stride, &offset);
		m_context->IASetIndexBuffer(mesh.ib, DXGI_FORMAT_R32_UINT, 0);

		m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		m_context->DrawIndexed(mesh.indexCount, 0, 0);
	}

	m_swapChain->Present(ref_renderConfig->enableVSync, 0);
	m_renderQueue.clear();
}

RenderTargetID D3D11Renderer::CreateRenderTarget(int width, int height, int format) {
	D3D11RenderTargetWrapper rt{};
	D3D11_TEXTURE2D_DESC	 desc = {};
	desc.Width					  = width;
	desc.Height					  = height;
	desc.MipLevels				  = 1;
	desc.ArraySize				  = 1;
	desc.Format					  = DXGI_FORMAT_R16G16B16A16_FLOAT;
	desc.SampleDesc.Count		  = 1;
	desc.Usage					  = D3D11_USAGE_DEFAULT;
	desc.BindFlags				  = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	ID3D11Texture2D *tex;
	m_device->CreateTexture2D(&desc, nullptr, &tex);
	m_device->CreateRenderTargetView(tex, nullptr, &rt.rtv);
	m_device->CreateShaderResourceView(tex, nullptr, &rt.srv);

	D3D11TextureWrapper tw{};
	tw.srv = rt.srv;
	if (tw.srv) tw.srv->AddRef();
	tw.texture	 = tex;
	tw.width	 = width;
	tw.height	 = height;
	rt.textureID = m_textures.Add(std::move(tw));

	return m_renderTargets.Add(std::move(rt));
}

TextureID D3D11Renderer::CreateTexture(const std::string &name, const unsigned char *data,
									   const TextureParameters &params) {
	D3D11TextureWrapper wrapper;
	wrapper.width  = params.width;
	wrapper.height = params.height;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width				  = params.width;
	desc.Height				  = params.height;
	desc.MipLevels			  = 1;
	desc.ArraySize			  = 1;
	desc.Format				  = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count	  = 1;
	desc.Usage				  = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags			  = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem				= data;
	initData.SysMemPitch			= params.width * 4;

	m_device->CreateTexture2D(&desc, &initData, &(wrapper.texture));
	m_device->CreateShaderResourceView(wrapper.texture, nullptr, &(wrapper.srv));

	return m_textures.Add(std::move(wrapper));
}

MeshID D3D11Renderer::CreateMesh(const std::string &name, const MeshData &meshData) {
	D3D11MeshWrapper mesh;
	mesh.indexCount = meshData.Indices.size();
	mesh.stride		= sizeof(Vertex);

	D3D11_BUFFER_DESC vbd = {static_cast<uint32_t>(meshData.Vertices.size()) * static_cast<uint32_t>(sizeof(Vertex)),
							 D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0};
	D3D11_SUBRESOURCE_DATA vData = {meshData.Vertices.data(), 0, 0};
	m_device->CreateBuffer(&vbd, &vData, &(mesh.vb));

	D3D11_BUFFER_DESC ibd = {static_cast<uint32_t>(meshData.Indices.size()) * static_cast<uint32_t>(sizeof(uint32_t)),
							 D3D11_USAGE_IMMUTABLE, D3D11_BIND_INDEX_BUFFER, 0};
	D3D11_SUBRESOURCE_DATA iData = {meshData.Indices.data(), 0, 0};
	m_device->CreateBuffer(&ibd, &iData, &(mesh.ib));

	return m_meshes.Add(std::move(mesh));
}

ShaderID D3D11Renderer::CreateShader(const ShaderType type, const std::filesystem::path &vsPath,
									 const std::filesystem::path &psPath) {
	if (D3D11Shader shader; shader.Initialize(m_device, type, vsPath, psPath) == ERROR_CODE::OK) {
		return m_shaders.Add(std::move(shader));
	}
	return INVALID_HANDLE;
}

MaterialID D3D11Renderer::CreateMaterial(ShaderID shaderID) {
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
		default: PE_LOG_ERROR("Not implemented!"); break;
	}

	Material	   mat;
	const uint32_t matID = static_cast<uint32_t>(m_materials.Data().size());

	mat.Initialize(this, matID, shaderID, bufferSize, matLayout);

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

	return m_materials.Add(std::move(mat));
}

void D3D11Renderer::SetRenderTargets(std::span<const RenderTargetID>(targets), RenderTargetID depthStencil) {
	std::vector<ID3D11RenderTargetView *> views;
	if (targets.data()) {
		for (const auto &target : targets) views.push_back(m_renderTargets.Get(target).rtv);
	} else {
		views.push_back(m_renderTargetView);
	}

	ID3D11DepthStencilView *dsv =
		(depthStencil == INVALID_HANDLE) ? m_depthStencilView : m_renderTargets.Get(depthStencil).dsv;
	m_context->OMSetRenderTargets(static_cast<uint32_t>(views.size()), views.data(), dsv);
}

void D3D11Renderer::ClearRenderTarget(RenderTargetID id, const float color[4]) {
	m_context->ClearRenderTargetView(m_renderTargets.Get(id).rtv, color);
}

void D3D11Renderer::ClearDepth(RenderTargetID id) {
	ID3D11DepthStencilView *dsv = (id == INVALID_HANDLE) ? m_depthStencilView : m_renderTargets.Get(id).dsv;
	m_context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

TextureID D3D11Renderer::GetRenderTargetTexture(RenderTargetID id) { return m_renderTargets.Get(id).textureID; }

void D3D11Renderer::UpdateGlobalBuffer(const CBPerPass &data) {
	m_perFrameBuffer->Update(m_context, &data);
	ID3D11Buffer *buff = m_perFrameBuffer->GetBuffer();

	m_context->VSSetConstantBuffers(0, 1, &buff);
	m_context->PSSetConstantBuffers(0, 1, &buff);
}

ERROR_CODE D3D11Renderer::UpdateMaterialTexture(MaterialID matID, TextureType typeIdx, TextureID texID) {
	// TODO: Implement
	return ERROR_CODE::OK;
}

ERROR_CODE D3D11Renderer::InitializeD3D11(const RenderConfig &config, const HWND hwnd) {
	uint32_t createDeviceFlags = 0;
	// IDXGIAdapter* adapter = nullptr;
	// if (const auto result = GetDedicatedGPUAdapter(adapter); result < ERROR_CODE::WARN_START)
	// return result;

#if defined(DEBUG) || defined(_DEBUG)
	EnumerateAdapters();
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
	D3D_FEATURE_LEVEL featureLevel;
	auto			  hResult = LOG_HR_RESULT(D3D11CreateDevice(nullptr,				   // dedicated gpu adapter
																D3D_DRIVER_TYPE_HARDWARE,  // driver type
																nullptr,				   // no software device
																createDeviceFlags,		   // flags
																nullptr,  // default feature level (D3D_FEATURE_LEVEL_11_0)
																0,		  // no SDK layers
																D3D11_SDK_VERSION,	// SDK version
																&m_device,			// device
																&featureLevel,		// feature level
																&m_context),
											  Utilities::LogLevel::Fatal,
											  "HRESULT failed while creating D3D11 device!");  // device context

	if (FAILED(hResult)) return ERROR_CODE::DX11_DEVICE_CREATION_FAILED;

	if (featureLevel != D3D_FEATURE_LEVEL_11_0) {
		PE_LOG_FATAL("Direct3D Feature Level 11 unsupported");
		return ERROR_CODE::DX11_DEVICE_CREATION_FAILED;
	}

	uint32_t msaaQuality4x = 0;
	hResult = LOG_HR_RESULT(m_device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &msaaQuality4x),
							Utilities::LogLevel::Fatal, "HRESULT failed while checking MSAA levels!");
	if (FAILED(hResult)) return ERROR_CODE::DX11_DEVICE_CREATION_FAILED;

	config.maxMsaaQuality = msaaQuality4x;
	DXGI_SWAP_CHAIN_DESC sd;
	if (config.enable4xMSAA) {
		sd.SampleDesc.Count	  = config.msaaCount;
		sd.SampleDesc.Quality = config.maxMsaaQuality - 1;
	} else {
		sd.SampleDesc.Count	  = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferDesc.Width					  = config.width;
	sd.BufferDesc.Height				  = config.height;
	sd.BufferDesc.RefreshRate.Numerator	  = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format				  = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering		  = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling				  = DXGI_MODE_SCALING_UNSPECIFIED;

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// 1 for double buffer, 2 for triple buffer
	sd.BufferCount	= 1;
	sd.OutputWindow = hwnd;
	sd.Windowed		= true;
	sd.SwapEffect	= DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags		= 0;

	IDXGIFactory *dxgiFactory = nullptr;
	hResult = LOG_HR_RESULT(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void **>(&dxgiFactory)),
							Utilities::LogLevel::Fatal, "HRESULT failed while creating DXGI factory.");
	if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

	hResult = LOG_HR_RESULT(dxgiFactory->CreateSwapChain(m_device, &sd, &m_swapChain), Utilities::LogLevel::Fatal,
							"HRESULT failed while creating swap chain.");
	if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

	hResult = LOG_HR_RESULT(m_swapChain->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void **>(&dxgiFactory)),
							Utilities::LogLevel::Fatal, "HRESULT failed while getting parent of swap chain!");
	if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

	SafeRelease(dxgiFactory);

	return OnResize(config);
}

ERROR_CODE D3D11Renderer::GetDedicatedGPUAdapter(IDXGIAdapter *&adapter) {
	IDXGIFactory *factory;
	HRESULT hResult = LOG_HR_RESULT(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void **>(&factory)),
									Utilities::LogLevel::Fatal, "HRESULT failed to creating DXGIFactory!");
	if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

	adapter		   = nullptr;
	SIZE_T maxVRAM = 0;

	for (UINT i = 0;; ++i) {
		IDXGIAdapter *current;
		if (factory->EnumAdapters(i, &current) == DXGI_ERROR_NOT_FOUND) break;

		DXGI_ADAPTER_DESC desc;
		hResult = LOG_HR_RESULT(current->GetDesc(&desc), Utilities::LogLevel::Fatal,
								"HRESULT failed to get DXGI_ADAPTER_DESC from adapter!");
		if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

		if (desc.DedicatedVideoMemory > maxVRAM) {
			maxVRAM = desc.DedicatedVideoMemory;

			if (adapter != nullptr) {
				adapter->Release();
			}

			adapter = current;
		} else {
			current->Release();
		}
	}

	SafeRelease(factory);

	if (adapter == nullptr) {
		return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;
	}

	return ERROR_CODE::OK;
}

ERROR_CODE D3D11Renderer::EnumerateAdapters() {
	IDXGIFactory *factory = nullptr;
	HRESULT hResult = LOG_HR_RESULT(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void **>(&factory)),
									Utilities::LogLevel::Fatal, "HRESULT failed to creating DXGIFactory!");
	if (FAILED(hResult)) {
		return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;
	}

	uint32_t	  adapterIndex	= 0;
	IDXGIAdapter *adapter		= nullptr;
	int			  totalAdapters = 0;

	while (factory->EnumAdapters(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC adapterDesc;
		hResult = LOG_HR_RESULT(adapter->GetDesc(&adapterDesc), Utilities::LogLevel::Fatal,
								"HRESULT failed to get adapter description!");
		if (FAILED(hResult)) return ERROR_CODE::DX11_PIPELINE_CREATION_FAILED;

		std::ostringstream ss;
		ss << "Adapter: " << Internal_WstringToUtf8(adapterDesc.Description) << std::endl;
		PE_LOG_INFO(ss.str());
		++totalAdapters;

		D3D_FEATURE_LEVEL	 featureLevel;
		ID3D11Device		*testDevice = nullptr;
		ID3D11DeviceContext *context	= nullptr;

		hResult = LOG_HR_RESULT(D3D11CreateDevice(adapter,	// dedicated gpu adapter
												  D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
												  &testDevice, &featureLevel, &context),
								Utilities::LogLevel::Fatal,
								"HRESULT failed while creating D3D11 device!");	 // device context
		if (FAILED(hResult)) {
			PE_LOG_INFO("*** D3D11 NOT SUPPORTED FOR ADAPTER");
			return ERROR_CODE::DX11_DEVICE_CREATION_FAILED;
		} else {
			PE_LOG_INFO("*** D3D11 SUPPORTED FOR ADAPTER");
			SafeRelease(testDevice);
			SafeRelease(context);
		}

		if (adapterIndex == 0) {
			uint32_t	 outputIndex = 0;
			IDXGIOutput *output		 = nullptr;
			int			 outputCount = 0;

			while (adapter->EnumOutputs(outputIndex, &output) != DXGI_ERROR_NOT_FOUND) {
				++outputCount;
				std::stringstream oss;
				oss << "*** OUTPUT " << outputIndex << ":";
				PE_LOG_INFO(oss.str());

				uint32_t numModes = 0;
				hResult = LOG_HR_RESULT(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, nullptr),
										Utilities::LogLevel::Fatal, "HRESULT failed while enumerating display modes!");
				if (FAILED(hResult)) return ERROR_CODE::DX11_DEVICE_CREATION_FAILED;

				if (numModes > 0) {
					DXGI_MODE_DESC *modeList = new DXGI_MODE_DESC[numModes];
					hResult =
						LOG_HR_RESULT(output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, modeList),
									  Utilities::LogLevel::Fatal, "HRESULT failed while enumerating display modes!");
					if (FAILED(hResult)) {
						delete[] modeList;
						return ERROR_CODE::DX11_DEVICE_CREATION_FAILED;
					}

					for (uint32_t i = 0; i < numModes; ++i) {
						std::stringstream mss;
						mss << "*** WIDTH = " << modeList[i].Width << " HEIGHT = " << modeList[i].Height
							<< " REFRESH = " << modeList[i].RefreshRate.Numerator << "/"
							<< modeList[i].RefreshRate.Denominator;

						PE_LOG_INFO(mss.str());
					}

					delete[] modeList;
				}

				SafeRelease(output);
				++outputIndex;
			}

			std::stringstream outss;
			outss << "*** NUM OUTPUTS FOR DEFAULT ADAPTER = " << outputCount;
			PE_LOG_INFO(outss.str());
		}

		SafeRelease(adapter);
		++adapterIndex;
	}

	std::stringstream totalss;
	totalss << "*** NUM ADAPTERS = " << totalAdapters;
	PE_LOG_INFO(totalss.str());
	SafeRelease(factory);
	return ERROR_CODE::OK;
}

ERROR_CODE D3D11Renderer::CreateConstantBuffers() {
	if (!m_perFrameBuffer) m_perFrameBuffer = new D3D11GPUBuffer();

	ERROR_CODE result;
	PE_ENSURE_INIT_SILENT(result, m_perFrameBuffer->Initialize(m_device,
															   sizeof(CBPerPass),			// ByteWidth
															   D3D11_BIND_CONSTANT_BUFFER,	// BindFlags
															   D3D11_USAGE_DYNAMIC,			// Usage
															   D3D11_CPU_ACCESS_WRITE		// CpuAccessFlags
															   ));

	if (!m_perObjectBuffer) m_perObjectBuffer = new D3D11GPUBuffer();

	PE_ENSURE_INIT_SILENT(result, m_perObjectBuffer->Initialize(m_device,
																sizeof(CBPerObject),		 // ByteWidth
																D3D11_BIND_CONSTANT_BUFFER,	 // BindFlags
																D3D11_USAGE_DYNAMIC,		 // Usage
																D3D11_CPU_ACCESS_WRITE		 // CpuAccessFlags
																));

	return result;
}

void D3D11Renderer::BeginPass(RenderPass pass) {
	ID3D11ShaderResourceView *nulls[3] = {nullptr};
	m_context->PSSetShaderResources(0, 3, nulls);
	m_context->PSSetShaderResources(10, 3, nulls);

	switch (pass) {
		case RenderPass::GBuffer: {
			SetRenderTargets(m_gbufferIDs, INVALID_HANDLE);
			float black[] = {0, 0, 0, 0};
			for (auto id : m_gbufferIDs) ClearRenderTarget(id, black);
			ClearDepth(INVALID_HANDLE);
		} break;
		case RenderPass::Lighting: {
			SetRenderTargets({}, INVALID_HANDLE);
			ID3D11ShaderResourceView *srvs[] = {m_renderTargets.Get(m_gbufferIDs[0]).srv,
												m_renderTargets.Get(m_gbufferIDs[1]).srv,
												m_renderTargets.Get(m_gbufferIDs[2]).srv};
			m_context->PSSetShaderResources(10, 3, srvs);
		} break;
		case RenderPass::Forward: {
			m_context->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

			float clearColor[] = {0.1f, 0.1f, 0.1f, 1.0f};	// Grey background
			m_context->ClearRenderTargetView(m_renderTargetView, clearColor);
			m_context->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
		} break;
		case RenderPass::UI: PE_LOG_ERROR("There is no UI rendering at the moment!"); break;
		default: break;
	}
}

void D3D11Renderer::CreateGBuffer(int width, int height) {
	m_renderTargets.Clear();

	m_gbufferIDs[0] = CreateRenderTarget(width, height, 1);
	m_gbufferIDs[1] = CreateRenderTarget(width, height, 1);
	m_gbufferIDs[2] = CreateRenderTarget(width, height, 1);
}

void D3D11Renderer::CreateSamplers() {
	D3D11_SAMPLER_DESC desc = {};
	desc.Filter				= D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	desc.MaxLOD									  = D3D11_FLOAT32_MAX;
	desc.ComparisonFunc							  = D3D11_COMPARISON_ALWAYS;
	m_device->CreateSamplerState(&desc, &m_linearSampler);

	desc.Filter	  = D3D11_FILTER_MIN_MAG_MIP_POINT;
	desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	m_device->CreateSamplerState(&desc, &m_pointSampler);

	ID3D11SamplerState *samplers[] = {m_linearSampler, m_pointSampler};
	m_context->PSSetSamplers(0, 2, samplers);
}
}  // namespace PE::Graphics::D3D11
#endif