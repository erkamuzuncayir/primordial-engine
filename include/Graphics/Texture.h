#pragma once
#include <vector>

#include "RenderTypes.h"

namespace PE::Graphics {
class Texture {
public:
	Texture() = default;

	Texture(const Texture &)			= delete;
	Texture &operator=(const Texture &) = delete;

	Texture(Texture &&other) noexcept : m_data(std::move(other.m_data)), m_params(other.m_params) {
		other.m_data.clear();
		other.m_params = {};
	}

	Texture &operator=(Texture &&other) noexcept {
		if (this != &other) {
			m_data	 = std::move(other.m_data);
			m_params = other.m_params;
			other.m_data.clear();
			other.m_params = {};
		}
		return *this;
	}

	~Texture() = default;

	void									  SetParameters(const TextureParameters params) { m_params = params; }
	[[nodiscard]] int						  GetWidth() const { return m_params.width; };
	[[nodiscard]] int						  GetHeight() const { return m_params.height; };
	[[nodiscard]] std::vector<unsigned char> &GetTextureData() { return m_data; };
	[[nodiscard]] TextureParameters			 &GetTextureParameters() { return m_params; };

private:
	std::vector<unsigned char> m_data;
	TextureParameters		   m_params{};
};
}  // namespace PE::Graphics