#pragma once

#ifdef PE_D3D11
#include "D3D11Shader.h"
#include "D3D11Utilities.h"
#include "Graphics/RenderTypes.h"

namespace PE::Graphics::D3D11 {
struct D3D11TextureWrapper {
	D3D11TextureWrapper() {}

	D3D11TextureWrapper(const D3D11TextureWrapper &)			= delete;
	D3D11TextureWrapper &operator=(const D3D11TextureWrapper &) = delete;

	D3D11TextureWrapper(D3D11TextureWrapper &&other) noexcept
		: srv(other.srv), texture(other.texture), width(other.width), height(other.height) {
		other.srv	  = nullptr;
		other.texture = nullptr;
	}

	D3D11TextureWrapper &operator=(D3D11TextureWrapper &&other) noexcept {
		if (this != &other) {
			SafeRelease(srv);
			SafeRelease(texture);

			srv		= other.srv;
			texture = other.texture;
			width	= other.width;
			height	= other.height;

			other.srv	  = nullptr;
			other.texture = nullptr;
		}
		return *this;
	}

	ID3D11ShaderResourceView *srv	  = nullptr;
	ID3D11Texture2D			 *texture = nullptr;
	int						  width	  = 0;
	int						  height  = 0;
};

struct D3D11MeshWrapper {
	D3D11MeshWrapper() {}

	D3D11MeshWrapper(const D3D11MeshWrapper &)			  = delete;
	D3D11MeshWrapper &operator=(const D3D11MeshWrapper &) = delete;

	D3D11MeshWrapper(D3D11MeshWrapper &&other) noexcept
		: vb(other.vb), ib(other.ib), indexCount(other.indexCount), stride(other.stride) {
		other.vb = nullptr;
		other.ib = nullptr;
	}

	D3D11MeshWrapper &operator=(D3D11MeshWrapper &&other) noexcept {
		if (this != &other) {
			SafeRelease(vb);
			SafeRelease(ib);

			vb		   = other.vb;
			ib		   = other.ib;
			indexCount = other.indexCount;
			stride	   = other.stride;

			other.vb = nullptr;
			other.ib = nullptr;
		}
		return *this;
	}

	ID3D11Buffer *vb		 = nullptr;
	ID3D11Buffer *ib		 = nullptr;
	uint32_t	  indexCount = 0;
	uint32_t	  stride	 = 0;
};

struct D3D11RenderTargetWrapper {
	D3D11RenderTargetWrapper() {}

	D3D11RenderTargetWrapper(const D3D11RenderTargetWrapper &)			  = delete;
	D3D11RenderTargetWrapper &operator=(const D3D11RenderTargetWrapper &) = delete;

	D3D11RenderTargetWrapper(D3D11RenderTargetWrapper &&other) noexcept
		: rtv(other.rtv), srv(other.srv), dsv(other.dsv), textureID(other.textureID) {
		other.rtv = nullptr;
		other.srv = nullptr;
		other.dsv = nullptr;
	}

	D3D11RenderTargetWrapper &operator=(D3D11RenderTargetWrapper &&other) noexcept {
		if (this != &other) {
			SafeRelease(rtv);
			SafeRelease(srv);
			SafeRelease(dsv);

			rtv		  = other.rtv;
			srv		  = other.srv;
			dsv		  = other.dsv;
			textureID = other.textureID;

			other.rtv = nullptr;
			other.srv = nullptr;
			other.dsv = nullptr;
		}
		return *this;
	}

	ID3D11RenderTargetView	 *rtv		= nullptr;
	ID3D11ShaderResourceView *srv		= nullptr;
	ID3D11DepthStencilView	 *dsv		= nullptr;
	TextureID				  textureID = INVALID_HANDLE;
};
}  // namespace PE::Graphics::D3D11
#endif