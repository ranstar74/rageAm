//
// File: boundgeometry.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "primitives.h"
#include "boundbase.h"
#include "rage/file/stream.h"
#include "rage/grcore/fvf.h"
#include "rage/math/vecv.h"

// TODO: Copy / Clone, CalcCGOffset, Inertia, Bounding Sphere

namespace rage
{
	// Visualization of triangle bound geometry
	//
	//
	//                 15                                                                      17                
	//                  ----------------------------------------------------------------------\
	//                 /                                                                 ----  \
	//                /                                                            -----/       \
	//               /                                                       -----/              \
	//              /                                                  -----/                     \
	//             /                                            ------/                            \
	//            /                                       -----/                                    \
	//           /                                  -----/                                           \
	//          /                             -----/                                                  \
	//         /                        -----/                                                         \
	//        /                  ------/                                                                \
	//       /             -----/                                                                        \
	//      /        -----/                                                                               \
	//     /   -----/                                                                                      \
	//    /---/                                                                                             \
	//   /-------------------------------------------------------------------------------------------------- - 
	//  16                                                                                                     18
	//
	// On diagram above you can see two triangles and vertex indices
	// Tri 1: 15 16 17
	// Tri 2: 16 18 17
	//
	// NOTE: All triangles facing camera must be winded in counter clock wise direction
	// 
	// What exactly neighbor means...?
	// If you look at drawing, those two triangles share single edge in common with vertex indices 16:17 (or 17:16)
	// - Neighbor polygon is assigned to every index in polygon relatively to first vertex index of the edge
	// 
	// Whats written above sounds terribly confusing but let's see with example above:
	// - In edge 16:17 the first vertex index is 16. in Tri 1, 16 is located at index 1 in vertices array
	// - In edge 17:16 the first vertex index is 17. in Tri 2, 17 is located at index 2 in vertices array
	// 
	// This meaning Tri 2 will be inserted at index 1 in vertex neighbor's array:
	// - Tri 1 ( Vertices: 15 16 17; Neighbor Polygons: -1 1 -1)
	// - Tri 2 ( Vertices: 16 18 17; Neighbor Polygons: -1 -1 0)
	//
	// -1 Indicates that vertex has no neighbor polygon
	//
	// Another way to see it is that from vertex X in direction X + 1 the neighbor polygon will be ALWAYS on the right side

	struct phOctantMap
	{
		static constexpr u32 MAX_OCTANTS = 8;

		u32  IndexCounts[8];
		u32* Indices[8];
		u32  IndexBuffer[1];
	};

	class phBoundPolyhedron : public phBound
	{
	protected:
		struct CompressedVertex { s16 X, Y, Z; };

		u32                        m_VerticesPad;
		pgCArray<CompressedVertex> m_CompressedShrunkVertices;
		u8                         m_Unused0;
		u8                         m_Unused1;
		u8                         m_NumPerVertexAttributes; // Only 1 is supported by GTA
		u16                        m_NumShrunkVertices;
		pgCArray<phPolygon>        m_Polygons;
		Vec3V                      m_UnQuantizeFactor;
		Vec3V                      m_BoundingBoxCenter;
		pgCArray<CompressedVertex> m_CompressedVertices;
		pgCArray<u32>              m_VertexAttributes;  // AKA vertex colors. Used to block rain?
		u32*                       m_OctantIndexCounts; // Number of vertices in each octant
		u32**                      m_OctantIndices;     // Vertex indices in each octant
		u32                        m_NumVertices;
		u32                        m_NumPolygons;
		u32                        m_Pad0[6];

		CompressedVertex CompressVertex(const Vec3V& vertex) const;

		void SetBoundingBox(const Vec3V& min, const Vec3V& max);
		void ComputeBoundingBoxCenter();
		// Computes neighbor polygon indices for each vertex in polygons
		void ComputeNeighbors() const;
		// Must be computed before compressing new vertices
		void ComputeUnQuantizeFactor();
		void CalculateBoundingSphere()
		{
			// TODO:
			m_RadiusAroundCentroid = 100.0f;
		}
		void CalculateVolumeDistribution()
		{
			// TODO: Test...
			m_VolumeDistribution = { 0.333f, 0.333f, 0.333f, 5.0f };
		}
		void ComputePolyArea() const;

		void SetOctantMap(phOctantMap*);
		void OctantMapDelete();
		void RecomputeOctantMap();
		void ComputeOctantMap();
		void OctantMapAllocateAndCopy(const u32* indexCounts, u32** indices);

	public:
		phBoundPolyhedron();
		phBoundPolyhedron(const datResource& rsc);

		void SetVertexColor(int index);
		u32  GetVertexColor(int index) const;

		phPolygon& GetPolygon(u16 i) const { return m_Polygons.Get()[i]; }
		void  DecompressPoly(const phPolygon& poly, Vec3V& v1, Vec3V& v2, Vec3V& v3, bool shrunk = false) const;
		Vec3V DecompressVertex(const CompressedVertex& vertex) const;
		Vec3V DecompressVertex(u32 index, bool shrunked = false) const;
		// Just alias for DecompressVertex
		Vec3V GetVertex(u32 index, bool shrunked = false) const { return DecompressVertex(index, shrunked); }

		// Reuses area to normalize the vector, faster but less precise
		Vec3V ComputePolygonNormalEstimate(const phPolygon& poly) const;
		Vec3V ComputePolygonNormal(const phPolygon& poly) const;

		// Computes average normals from adjacent polygons
		void ComputeVertexNormals(Vec3V* outNormals) const;
		void DecompressVertices(Vec3V* outVertices) const;
		void GetIndices(u16* outIndices) const;

		bool IsPolygonal() const override { return true; }

		u32 GetVertexCount() const { return m_NumVertices; }
		u32 GetPolygonCount() const { return m_NumPolygons; }
		u32 GetIndexCount() const { return m_NumPolygons * 3; }

		phOctantMap* GetOctantMap() const;

		virtual void CalcCGOffset(Vec3V& outOffset) { outOffset = VEC_ZERO; } // TODO: Used for cloth?

		virtual phMaterialMgr::Id GetMaterialId(int partIndex) = 0;
		virtual phMaterialMgr::Id GetPolygonMaterialIndex(int polygon) = 0;
		virtual void			  SetPolygonMaterialIndex(int polygon, int materialIndex) = 0;

		virtual bool HasOpenEdges() { return false; }
	};

	class phBoundGeometry : public phBoundPolyhedron
	{
	protected:
		phMaterialMgr::Id* m_Materials;
		u32*               m_MaterialColors;
		u8                 m_Pad1[4];
		float*             m_SecondSurfaceVertexDisplacements;
		int                m_NumSecondSurfaceVertexDisplacements; // Equals to m_NumVertices
		u8*                m_PolygonToMaterial;
		u8                 m_NumMaterials;
		u8                 m_NumMaterialColors;
		u8                 m_Pad2[14];

		// Shrinks vertices along average normal by margin distance
		void ShrinkVerticesByMargin(float margin, Vec3V* outVertices) const;
		// Shrinks vertices by weighted normal computed from neighbor polygons
		void ShrinkPolysByMargin(float margin, Vec3V* outVertices) const;
		// T specifies amount of interpolation from polygon to vertex shrink
		void ShrinkPolysOrVertsByMargin(float margin, float t, Vec3V* outVertices) const;
		// Attempts to shrink and returns if any polygon intersects or not
		bool TryShrinkByMargin(float margin, float t, Vec3V* outShrunkVertices) const;

	public:
		phBoundGeometry();
		phBoundGeometry(const datResource& rsc);
		phBoundGeometry(const spdAABB& bb, const Vector3* vertices, const u16* indices, u32 vertexCount, u32 indexCount);

		void PostLoadCompute() override;

		void SetCentroidOffset(const Vec3V& offset) override;
		void ShiftCentroidOffset(const Vec3V& offset) override;

		int				  GetPolygonMaterial(int partIndex) const { AM_ASSERTS(partIndex < m_NumPolygons); return m_PolygonToMaterial[partIndex]; }
		u32			      GetMaterialColor(int partIndex) const { AM_ASSERTS(partIndex < m_NumMaterialColors); return m_MaterialColors[partIndex]; }
		int				  GetNumMaterialColors() const { return m_NumMaterialColors; }
		int               GetNumMaterials() const override { return m_NumMaterials; }
		phMaterial*       GetMaterial(int partIndex) const override;
		void              SetMaterial(phMaterialMgr::Id materialId, int partIndex) override;
		phMaterialMgr::Id GetMaterialIdFromPartIndexAndComponent(int partIndex, int boundIndex = BOUND_PARTS_ALL) const override;

		void CalculateExtents() override;

		bool HasOpenEdges() override { return false; } // TODO: ...

		virtual void GetMaterialName(int partIndex, char* buffer, int bufferSize) const;
		virtual void ScaleSize(float x, float y, float z) { } // TODO: ...
		virtual bool ComponentsStreamed() { return false; } // Deprecated & Unused

		void SetMarginAndShrink(float margin = PH_DEFAULT_MARGIN, float t = 0.0f);

		// Decompresses all vertices and stores them in non-SIMD vector
		amUPtr<Vector3[]> GetVertices(bool shrunkedVertices = false) const;
		// Converts existing triangle polygons to index array
		amUPtr<u16[]> GetIndices() const;
		// Creates vertex buffer that can be used for rendering
		amUPtr<char> ComposeVertexBuffer(const grcVertexFormatInfo& vertexInfo) const;

		phMaterialMgr::Id GetMaterialId(int partIndex) override;
		phMaterialMgr::Id GetPolygonMaterialIndex(int polygon) override;
		void			  SetPolygonMaterialIndex(int polygon, int materialIndex) override;

		// ----- Debugging -----

		// Writes all geometry triangles to given stream in OBJ format
		void WriteObj(const fiStreamPtr& stream, bool shrunkedVertices = false) const;
		void WriteObj(ConstString path, bool shrunkedVertices = false) const;
		// Prints all octant indices & counts
		void PrintOctantMap(bool printIndices = false) const;
	};
}
