#pragma once

#include <vector>

#include "Graphics/Vertex.h"

namespace PE::Graphics {
class Mesh {
public:
	Mesh() = default;

	Mesh(const Mesh &)			  = delete;
	Mesh &operator=(const Mesh &) = delete;

	Mesh(Mesh &&other) noexcept : m_vertices(std::move(other.m_vertices)), m_indices(std::move(other.m_indices)) {}

	Mesh &operator=(Mesh &&other) noexcept {
		if (this != &other) {
			m_vertices = std::move(other.m_vertices);
			m_indices  = std::move(other.m_indices);
		}
		return *this;
	}

	~Mesh() { Shutdown(); }

	void Initialize(const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices) {
		m_vertices = vertices;
		m_indices  = indices;
	}

	void Shutdown() {
		m_vertices.clear();
		m_indices.clear();
	}

	[[nodiscard]] const std::vector<Vertex>	  &GetVertices() const { return m_vertices; }
	[[nodiscard]] const std::vector<uint32_t> &GetIndices() const { return m_indices; }

	[[nodiscard]] uint32_t GetVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }
	[[nodiscard]] uint32_t GetIndexCount() const { return static_cast<uint32_t>(m_indices.size()); }

	[[nodiscard]] const Vertex	 *GetVertexData() const { return m_vertices.data(); }
	[[nodiscard]] const uint32_t *GetIndexData() const { return m_indices.data(); }

private:
	std::vector<Vertex>	  m_vertices;
	std::vector<uint32_t> m_indices;
};
}  // namespace PE::Graphics