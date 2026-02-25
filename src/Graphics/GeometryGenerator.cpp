#include "Graphics/GeometryGenerator.h"

#include <algorithm>

#include "Math/Math.h"

namespace PE::Graphics {
void GeometryGenerator::CreateBox(const float width, const float height, const float depth, MeshData &meshData) {
	//
	// Create the vertices.
	//

	Vertex v[24];

	const float w2 = 0.5f * width;
	const float h2 = 0.5f * height;
	const float d2 = 0.5f * depth;

	// Fill in the front face vertex data.
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8]  = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9]  = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	meshData.Vertices.assign(&v[0], &v[24]);

	//
	// Create the indices.
	//

	uint32_t i[36];

	// Fill in the front face index data
	i[0] = 0;
	i[1] = 1;
	i[2] = 2;
	i[3] = 0;
	i[4] = 2;
	i[5] = 3;

	// Fill in the back face index data
	i[6]  = 4;
	i[7]  = 5;
	i[8]  = 6;
	i[9]  = 4;
	i[10] = 6;
	i[11] = 7;

	// Fill in the top face index data
	i[12] = 8;
	i[13] = 9;
	i[14] = 10;
	i[15] = 8;
	i[16] = 10;
	i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12;
	i[19] = 13;
	i[20] = 14;
	i[21] = 12;
	i[22] = 14;
	i[23] = 15;

	// Fill in the left face index data
	i[24] = 16;
	i[25] = 17;
	i[26] = 18;
	i[27] = 16;
	i[28] = 18;
	i[29] = 19;

	// Fill in the right face index data
	i[30] = 20;
	i[31] = 21;
	i[32] = 22;
	i[33] = 20;
	i[34] = 22;
	i[35] = 23;

	meshData.Indices.assign(&i[0], &i[36]);
}

void GeometryGenerator::CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount, MeshData &meshData) {
	meshData.Vertices.clear();
	meshData.Indices.clear();

	//
	// Compute the vertices stating at the top pole and moving down the stacks.
	//

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	meshData.Vertices.push_back(topVertex);

	// Use constants from our math abstraction
	float phiStep	= Math::PI / stackCount;
	float thetaStep = Math::TAU / sliceCount;  // Replaced 2.0f * PI

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (uint32_t i = 1; i <= stackCount - 1; ++i) {
		float phi = i * phiStep;

		// Vertices of ring.
		for (uint32_t j = 0; j <= sliceCount; ++j) {
			float theta = j * thetaStep;

			Vertex v;

			// spherical to cartesian
			v.Position.x = radius * sinf(phi) * cosf(theta);
			v.Position.y = radius * cosf(phi);
			v.Position.z = radius * sinf(phi) * sinf(theta);

			// Partial derivative of P with respect to theta
			v.Tangent.x = -radius * sinf(phi) * sinf(theta);
			v.Tangent.y = 0.0f;
			v.Tangent.z = +radius * sinf(phi) * cosf(theta);

			v.Tangent = Math::Normalize(v.Tangent);
			v.Normal  = Math::Normalize(v.Position);

			v.TexC.x = theta / Math::TAU;  // Use Math::TAU (which is 2*PI)
			v.TexC.y = phi / Math::PI;

			meshData.Vertices.push_back(v);
		}
	}

	meshData.Vertices.push_back(bottomVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (uint32_t i = 1; i <= sliceCount; ++i) {
		meshData.Indices.push_back(0);
		meshData.Indices.push_back(i + 1);
		meshData.Indices.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	uint32_t baseIndex		 = 1;
	uint32_t ringVertexCount = sliceCount + 1;
	for (uint32_t i = 0; i < stackCount - 2; ++i) {
		for (uint32_t j = 0; j < sliceCount; ++j) {
			meshData.Indices.push_back(baseIndex + i * ringVertexCount + j);
			meshData.Indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.Indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			meshData.Indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			meshData.Indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.Indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole vertex was added last.
	uint32_t southPoleIndex = static_cast<uint32_t>(meshData.Vertices.size()) - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32_t i = 0; i < sliceCount; ++i) {
		meshData.Indices.push_back(southPoleIndex);
		meshData.Indices.push_back(baseIndex + i);
		meshData.Indices.push_back(baseIndex + i + 1);
	}
}

void GeometryGenerator::CreateGeosphere(const float radius, uint32_t numSubdivisions, MeshData &meshData) {
	// Use std::min from <algorithm> header
	numSubdivisions = std::min(numSubdivisions, 5u);

	// Approximate a sphere by tessellating an icosahedron.

	constexpr float X = 0.525731f;
	constexpr float Z = 0.850651f;

	Math::Vec3 pos[12] = {Math::Vec3(-X, 0.0f, Z), Math::Vec3(X, 0.0f, Z),	 Math::Vec3(-X, 0.0f, -Z),
						  Math::Vec3(X, 0.0f, -Z), Math::Vec3(0.0f, Z, X),	 Math::Vec3(0.0f, Z, -X),
						  Math::Vec3(0.0f, -Z, X), Math::Vec3(0.0f, -Z, -X), Math::Vec3(Z, X, 0.0f),
						  Math::Vec3(-Z, X, 0.0f), Math::Vec3(Z, -X, 0.0f),	 Math::Vec3(-Z, -X, 0.0f)};

	unsigned long k[60] = {1, 4,  0, 4, 9, 0,  4, 5, 9,	 8, 5, 4,  1,  8,  4, 1, 10, 8,	 10, 3,
						   8, 8,  3, 5, 3, 2,  5, 3, 7,	 2, 3, 10, 7,  10, 6, 7, 6,	 11, 7,	 6,
						   0, 11, 6, 1, 0, 10, 1, 6, 11, 0, 9, 2,  11, 9,  5, 2, 9,	 11, 2,	 7};

	meshData.Vertices.resize(12);
	meshData.Indices.resize(60);

	for (uint32_t i = 0; i < 12; ++i) meshData.Vertices[i].Position = pos[i];

	for (uint32_t i = 0; i < 60; ++i) meshData.Indices[i] = k[i];

	for (uint32_t i = 0; i < numSubdivisions; ++i) Subdivide(meshData);

	// Project vertices onto sphere and scale.
	for (uint32_t i = 0; i < meshData.Vertices.size(); ++i) {
		// Project onto unit sphere.
		Math::Vec3 n = Math::Normalize(meshData.Vertices[i].Position);
		// Project onto sphere.
		const Math::Vec3 p = n * radius;  // Scaling a vector is just multiplication

		meshData.Vertices[i].Position = p;
		meshData.Vertices[i].Normal	  = n;

		float		theta = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);
		const float phi	  = acosf(meshData.Vertices[i].Position.y / radius);

		// Handle pole singularity
		if (theta < 0.0f) theta += Math::TAU;  // (2*PI)

		meshData.Vertices[i].TexC.x = theta / Math::TAU;  // Use Math::TAU
		meshData.Vertices[i].TexC.y = phi / Math::PI;

		// Partial derivative of P with respect to theta
		meshData.Vertices[i].Tangent.x = -radius * sinf(phi) * sinf(theta);
		meshData.Vertices[i].Tangent.y = 0.0f;
		meshData.Vertices[i].Tangent.z = +radius * sinf(phi) * cosf(theta);

		meshData.Vertices[i].Tangent = Math::Normalize(meshData.Vertices[i].Tangent);
	}
}

void GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount,
									   uint32_t stackCount, MeshData &meshData) {
	meshData.Vertices.clear();
	meshData.Indices.clear();

	//
	// Build Stacks.
	//
	float stackHeight = height / stackCount;
	// Amount to increment radius as we move up each stack level from bottom to top.
	float	 radiusStep = (topRadius - bottomRadius) / stackCount;
	uint32_t ringCount	= stackCount + 1;

	// Compute vertices for each stack ring starting at the bottom and moving up.
	for (uint32_t i = 0; i < ringCount; ++i) {
		float y = -0.5f * height + i * stackHeight;
		float r = bottomRadius + i * radiusStep;
		// vertices of ring
		float dTheta = Math::TAU / sliceCount;

		for (uint32_t j = 0; j <= sliceCount; ++j) {
			Vertex vertex;

			float c = cosf(j * dTheta);
			float s = sinf(j * dTheta);

			vertex.Position = Math::Vec3(r * c, y, r * s);

			vertex.TexC.x = static_cast<float>(j) / sliceCount;
			vertex.TexC.y = 1.0f - static_cast<float>(i) / stackCount;

			// Cylinder can be parameterized as follows, where we introduce v
			// parameter that goes in the same direction as the v tex-coord
			// so that the bitangent goes in the same direction as the v tex-coord.
			//   Let r0 be the bottom radius and let r1 be the top radius.
			//   y(v) = h - hv for v in [0,1].
			//   r(v) = r1 + (r0-r1)v
			//
			//   x(t, v) = r(v)*cos(t)
			//   y(t, v) = h - hv
			//   z(t, v) = r(v)*sin(t)
			//
			//  dx/dt = -r(v)*sin(t)
			//  dy/dt = 0
			//  dz/dt = +r(v)*cos(t)
			//
			//  dx/dv = (r0-r1)*cos(t)
			//  dy/dv = -h
			//  dz/dv = (r0-r1)*sin(t)

			// This is unit length.
			vertex.Tangent = Math::Vec3(-s, 0.0f, c);

			float	   dr = bottomRadius - topRadius;
			Math::Vec3 bitangent(dr * c, -height, dr * s);

			Math::Vec3 T  = vertex.Tangent;
			Math::Vec3 B  = bitangent;
			Math::Vec3 N  = Math::Normalize(Math::Cross(T, B));
			vertex.Normal = N;

			meshData.Vertices.push_back(vertex);
		}
	}

	// Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different.
	uint32_t ringVertexCount = sliceCount + 1;

	// Compute indices for each stack.
	for (uint32_t i = 0; i < stackCount; ++i) {
		for (uint32_t j = 0; j < sliceCount; ++j) {
			meshData.Indices.push_back(i * ringVertexCount + j);
			meshData.Indices.push_back((i + 1) * ringVertexCount + j);
			meshData.Indices.push_back((i + 1) * ringVertexCount + j + 1);

			meshData.Indices.push_back(i * ringVertexCount + j);
			meshData.Indices.push_back((i + 1) * ringVertexCount + j + 1);
			meshData.Indices.push_back(i * ringVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
	BuildCylinderBottomCap(bottomRadius, topRadius, height, sliceCount, stackCount, meshData);
}

void GeometryGenerator::CreateGrid(const float width, const float depth, const uint32_t m, const uint32_t n,
								   MeshData &meshData) {
	const uint32_t vertexCount = m * n;
	const uint32_t faceCount   = (m - 1) * (n - 1) * 2;

	//
	// Create the vertices.
	//

	const float halfWidth = 0.5f * width;
	const float halfDepth = 0.5f * depth;

	const float dx = width / (n - 1);
	const float dz = depth / (m - 1);

	const float du = 1.0f / (n - 1);
	const float dv = 1.0f / (m - 1);

	meshData.Vertices.resize(vertexCount);
	for (uint32_t i = 0; i < m; ++i) {
		float z = halfDepth - i * dz;
		for (uint32_t j = 0; j < n; ++j) {
			float x = -halfWidth + j * dx;

			meshData.Vertices[i * n + j].Position = Math::Vec3(x, 0.0f, z);
			meshData.Vertices[i * n + j].Normal	  = Math::Vec3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[i * n + j].Tangent  = Math::Vec3(1.0f, 0.0f, 0.0f);

			// Stretch texture over grid.
			meshData.Vertices[i * n + j].TexC.x = j * du;
			meshData.Vertices[i * n + j].TexC.y = i * dv;
		}
	}

	//
	// Create the indices.
	//

	meshData.Indices.resize(faceCount * 3);	 // 3 indices per face

	// Iterate over each quad and compute indices.
	uint32_t k = 0;
	for (uint32_t i = 0; i < m - 1; ++i) {
		for (uint32_t j = 0; j < n - 1; ++j) {
			meshData.Indices[k]		= i * n + j;
			meshData.Indices[k + 1] = i * n + j + 1;
			meshData.Indices[k + 2] = (i + 1) * n + j;

			meshData.Indices[k + 3] = (i + 1) * n + j;
			meshData.Indices[k + 4] = i * n + j + 1;
			meshData.Indices[k + 5] = (i + 1) * n + j + 1;

			k += 6;	 // next quad
		}
	}
}

void GeometryGenerator::CreateQuad(const float width, const float height, MeshData &meshData) {
	meshData.Vertices.clear();
	meshData.Indices.clear();

	meshData.Vertices.resize(4);
	meshData.Indices.resize(6);

	// Calculate half the width and half the height.
	// This ensures that the (0,0,0) point remains exactly in the centre.
	const float w2 = 0.5f * width;
	const float h2 = 0.5f * height;

	// --- VERTICES ---
	// Order: Bottom-Left, Top-Left, Top-Right, Bottom-Right
	// Position (XYZ), Normal (XYZ), Tangent (XYZ), UV (UV)
	// We set the Z axis (Depth) to 0.0f because the depth should be managed by the Transform Component (Position.z).

	// Bottom-Left
	meshData.Vertices[0] = Vertex(-w2, -h2, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	// Top-Left
	meshData.Vertices[1] = Vertex(-w2, h2, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);

	// Top-Right
	meshData.Vertices[2] = Vertex(w2, h2, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Bottom-Right
	meshData.Vertices[3] = Vertex(w2, -h2, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// --- INDICES ---
	// Counter-clockwise (CCW) winding sequence

	// Triangle 1 (Bottom Left -> Top Left -> Top Right)
	meshData.Indices[0] = 0;
	meshData.Indices[1] = 1;
	meshData.Indices[2] = 2;

	// Triangle 2 (Bottom Left -> Top Right -> Bottom Right)
	meshData.Indices[3] = 0;
	meshData.Indices[4] = 2;
	meshData.Indices[5] = 3;
}

void GeometryGenerator::CreateFullscreenQuad(MeshData &meshData) {
	meshData.Vertices.resize(4);
	meshData.Indices.resize(6);

	// Position coordinates specified in NDC space.
	meshData.Vertices[0] = Vertex(-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	meshData.Vertices[1] = Vertex(-1.0f, +1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	meshData.Vertices[2] = Vertex(+1.0f, +1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	meshData.Vertices[3] = Vertex(+1.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	meshData.Indices[0] = 0;
	meshData.Indices[1] = 1;
	meshData.Indices[2] = 2;

	meshData.Indices[3] = 0;
	meshData.Indices[4] = 2;
	meshData.Indices[5] = 3;
}

void GeometryGenerator::Subdivide(MeshData &meshData) {
	// Save a copy of the input geometry.
	MeshData inputCopy = meshData;

	meshData.Vertices.resize(0);
	meshData.Indices.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	uint32_t numTris = inputCopy.Indices.size() / 3;
	for (uint32_t i = 0; i < numTris; ++i) {
		Vertex v0 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 0]];
		Vertex v1 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 1]];
		Vertex v2 = inputCopy.Vertices[inputCopy.Indices[i * 3 + 2]];

		//
		// Generate the midpoints.
		//

		Vertex m0, m1, m2;

		// Use Math::Vector3 constructors (or just + and * operators)
		m0.Position = (v0.Position + v1.Position) * 0.5f;
		m1.Position = (v1.Position + v2.Position) * 0.5f;
		m2.Position = (v0.Position + v2.Position) * 0.5f;

		//
		// Add new geometry.
		//

		meshData.Vertices.push_back(v0);  // 0
		meshData.Vertices.push_back(v1);  // 1
		meshData.Vertices.push_back(v2);  // 2
		meshData.Vertices.push_back(m0);  // 3
		meshData.Vertices.push_back(m1);  // 4
		meshData.Vertices.push_back(m2);  // 5

		meshData.Indices.push_back(i * 6 + 0);
		meshData.Indices.push_back(i * 6 + 3);
		meshData.Indices.push_back(i * 6 + 5);

		meshData.Indices.push_back(i * 6 + 3);
		meshData.Indices.push_back(i * 6 + 4);
		meshData.Indices.push_back(i * 6 + 5);

		meshData.Indices.push_back(i * 6 + 5);
		meshData.Indices.push_back(i * 6 + 4);
		meshData.Indices.push_back(i * 6 + 2);

		meshData.Indices.push_back(i * 6 + 3);
		meshData.Indices.push_back(i * 6 + 1);
		meshData.Indices.push_back(i * 6 + 4);
	}
}

void GeometryGenerator::BuildCylinderTopCap(float bottomRadius, const float topRadius, const float height,
											const uint32_t sliceCount, uint32_t stackCount, MeshData &meshData) {
	const auto	baseIndex = static_cast<uint32_t>(meshData.Vertices.size());
	const float y		  = 0.5f * height;
	const float dTheta	  = Math::TAU / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (uint32_t i = 0; i <= sliceCount; ++i) {
		const float x = topRadius * cosf(i * dTheta);
		const float z = topRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		const float u = x / height + 0.5f;
		const float v = z / height + 0.5f;

		meshData.Vertices.emplace_back(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v);
	}

	// Cap center vertex.
	meshData.Vertices.emplace_back(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f);

	// Index of center vertex.
	const uint32_t centerIndex = static_cast<uint32_t>(meshData.Vertices.size()) - 1;

	for (uint32_t i = 0; i < sliceCount; ++i) {
		meshData.Indices.push_back(centerIndex);
		meshData.Indices.push_back(baseIndex + i + 1);
		meshData.Indices.push_back(baseIndex + i);
	}
}

void GeometryGenerator::BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount,
											   uint32_t stackCount, MeshData &meshData) {
	//
	// Build bottom cap.
	//

	const auto	baseIndex = static_cast<uint32_t>(meshData.Vertices.size());
	const float y		  = -0.5f * height;

	// vertices of ring
	const float dTheta = Math::TAU / sliceCount;

	for (uint32_t i = 0; i <= sliceCount; ++i) {
		const float x = bottomRadius * cosf(i * dTheta);
		const float z = bottomRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area
		// proportional to base.
		const float u = x / height + 0.5f;
		const float v = z / height + 0.5f;

		meshData.Vertices.emplace_back(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v);
	}

	// Cap center vertex.
	meshData.Vertices.emplace_back(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f);

	// Cache the index of center vertex.
	const uint32_t centerIndex = static_cast<uint32_t>(meshData.Vertices.size()) - 1;

	for (uint32_t i = 0; i < sliceCount; ++i) {
		meshData.Indices.push_back(centerIndex);
		meshData.Indices.push_back(baseIndex + i);
		meshData.Indices.push_back(baseIndex + i + 1);
	}
}
}  // namespace PE::Graphics