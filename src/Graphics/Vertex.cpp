#include "Graphics/Vertex.h"

namespace PE::Graphics {
Vertex::Vertex() : Position(0, 0, 0), Normal(0, 0, 0), Tangent(0, 0, 0), TexC(0, 0) {}

Vertex::Vertex(Math::Vector3 position, Math::Vector3 normal, Math::Vector3 tangent, Math::Vector2 texcoord) {
	this->Position = position;
	this->Normal   = normal;
	this->Tangent  = tangent;
	this->TexC	   = texcoord;
}

Vertex::Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u,
			   float v)
	: Position(px, py, pz), Normal(nx, ny, nz), Tangent(tx, ty, tz), TexC(u, v) {}
}  // namespace PE::Graphics