#pragma once

#include "RenderTypes.h"

namespace PE::Graphics {
class GeometryGenerator {
public:
	GeometryGenerator(const GeometryGenerator &)			= delete;
	GeometryGenerator &operator=(const GeometryGenerator &) = delete;
	GeometryGenerator(GeometryGenerator &&)					= delete;
	GeometryGenerator &operator=(GeometryGenerator &&)		= delete;

	///< summary>
	/// Creates a box centered at the origin with the given dimensions.
	///</summary>
	static void CreateBox(float width, float height, float depth, MeshData &meshData);

	///< summary>
	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	///</summary>
	static void CreateSphere(float radius, uint32_t sliceCount, uint32_t stackCount, MeshData &meshData);

	///< summary>
	/// Creates a geosphere centered at the origin with the given radius.  The
	/// depth controls the level of tessellation.
	///</summary>
	static void CreateGeosphere(float radius, uint32_t numSubdivisions, MeshData &meshData);

	///< summary>
	/// Creates a cylinder parallel to the y-axis, and centered about the origin.
	/// The bottom and top radius can vary to form various cone shapes rather than true
	/// cylinders. The slices and stacks parameters control the degree of tessellation.
	///</summary>
	static void CreateCylinder(float bottomRadius, float topRadius, float height, uint32_t sliceCount,
							   uint32_t stackCount, MeshData &meshData);

	///< summary>
	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	///</summary>
	static void CreateGrid(float width, float depth, uint32_t m, uint32_t n, MeshData &meshData);

	///< summary>
	/// Creates a quad in the xy-plane centered at the origin with the
	/// specified width and height.
	///</summary>
	static void CreateQuad(float width, float height, MeshData &meshData);

	///< summary>
	/// Creates a quad covering the screen in NDC coordinates.  This is useful for
	/// postprocessing effects.
	///</summary>
	static void CreateFullscreenQuad(MeshData &meshData);

private:
	static void Subdivide(MeshData &meshData);
	static void BuildCylinderTopCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount,
									uint32_t stackCount, MeshData &meshData);
	static void BuildCylinderBottomCap(float bottomRadius, float topRadius, float height, uint32_t sliceCount,
									   uint32_t stackCount, MeshData &meshData);
};
}  // namespace PE::Graphics