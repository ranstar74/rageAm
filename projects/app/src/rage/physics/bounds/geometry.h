//
// File: geometry.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

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

	static constexpr u16 INVALID_NEIGHBOR = u16(-1);

	enum ePolygonType : u8
	{
		PT_TRIANGLE = 0
	};

	// In RDR3 was renamed to phPrimTriangle, phPolygon appears in RDR1 Switch symbols
	class phPrimTriangle
	{
	public: // TEST
		static constexpr u16 VERTEX_INDEX_MASK = 0x7FFF;
		static constexpr u32 TYPE_MASK = 0x7; // 3 lowest bits
		static constexpr u32 AREA_MASK = 0xFFFFFF << 8; // 24 high bits

		// Bits 3-7 are seem to be unused, although masked in native code (0xF8)
		u32	m_AreaAndType;
		u16	m_VertexIndices[3]; // TODO: Look into high bit
		u16	m_NeighboringPolygons[3];

		void SetType(u8 type)
		{
			m_AreaAndType &= ~TYPE_MASK;
			m_AreaAndType |= type & TYPE_MASK;
		}

	public:
		phPrimTriangle()
		{
			if (datResource::IsBuilding())
				return;

			m_AreaAndType = 0;
			memset(m_VertexIndices, 0, sizeof m_VertexIndices);
			memset(m_NeighboringPolygons, 0, sizeof m_NeighboringPolygons);
		}

		phPrimTriangle(u16 vi1, u16 vi2, u16 vi3) : phPrimTriangle()
		{
			m_VertexIndices[0] = vi1;
			m_VertexIndices[1] = vi2;
			m_VertexIndices[2] = vi3;
		}

		ePolygonType GetType() const { return ePolygonType(m_AreaAndType & TYPE_MASK); }

		// Non-triangle types are used only on BVH
		bool IsTriangle() const { return GetType() == PT_TRIANGLE; }

		// Including unknown high bit, used in shrink by poly as key in set
		u16 GetVertexID(u32 i) const { return m_VertexIndices[i]; }

		u16 GetVertexIndex(u32 i) const { return m_VertexIndices[i] & VERTEX_INDEX_MASK; }
		u16 GetNeighborIndex(u32 i) const { return m_NeighboringPolygons[i]; }
		void SetNeighborIndex(u32 i, u32 neighbor) { m_NeighboringPolygons[i] = neighbor; }

		bool UsesVertexIndex(u32 index) const
		{
			if (GetVertexIndex(0) == index) return true;
			if (GetVertexIndex(1) == index) return true;
			if (GetVertexIndex(2) == index) return true;
			return false;
		}

		// Sets all neighbor indices to invalid
		void ResetNeighboors()
		{
			for (u16& m_NeighboringPolygon : m_NeighboringPolygons)
				m_NeighboringPolygon = INVALID_NEIGHBOR;
		}

		// Type bits are packed with float value, be careful

		float GetArea() const
		{
			u32 bits = m_AreaAndType & AREA_MASK;
			return *reinterpret_cast<float*>(&bits);
		}

		void SetArea(float area)
		{
			u32 bits = *reinterpret_cast<u32*>(&area);
			m_AreaAndType &= ~AREA_MASK;
			m_AreaAndType |= bits & AREA_MASK;
		}
	};

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

		using Polygons = pgCArray<phPrimTriangle>;
		using PackedVerts = pgCArray<CompressedVertex>;
		using OctCounts = u32*;
		using OctIndices = u32**;

		u32			m_VerticesPad;
		PackedVerts	m_CompressedShrunkVertices;
		u16			m_Unknown80;
		u8			m_Unknown82;
		u16			m_NumShrunkVertices;
		Polygons	m_Polygons;
		Vec3V		m_UnQuantizeFactor;
		Vec3V		m_BoundingBoxCenter;
		PackedVerts	m_CompressedVertices;
		u64			m_UnknownB8; // Pointer
		OctCounts	m_OctantIndexCounts;
		OctIndices  m_OctantIndices;
		u32			m_NumVertices;
		u32			m_NumPolygons;
		u64			m_UnknownD8;
		u64			m_UnknownE0;
		u64			m_UnknownE8;

		CompressedVertex CompressVertex(const Vec3V& vertex) const;

		void SetBoundingBox(const Vec3V& min, const Vec3V& max)
		{
			m_BoundingBoxMin = min;
			m_BoundingBoxMax = max;

			ComputeBoundingBoxCenter();
			ComputeUnQuantizeFactor();
			CalculateBoundingSphere();
		}

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
			// TODO:
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

		phPrimTriangle& GetPolygon(u16 i) const { return m_Polygons.Get()[i]; }
		void DecompressPoly(const phPrimTriangle& poly, Vec3V& v1, Vec3V& v2, Vec3V& v3, bool shrunk = false) const;
		Vec3V DecompressVertex(const CompressedVertex& vertex) const;
		Vec3V DecompressVertex(u32 index, bool shrunked = false) const;

		// Reuses area to normalize the vector, faster but less precise
		Vec3V ComputePolygonNormalEstimate(const phPrimTriangle& poly) const;
		Vec3V ComputePolygonNormal(const phPrimTriangle& poly) const;

		// Computes average normals from adjacent polygons
		void ComputeVertexNormals(Vec3V* outNormals) const;
		void DecompressVertices(Vec3V* outVertices) const;
		void GetIndices(u16* outIndices) const;

		bool IsPolygonal() const override { return true; }

		u32 GetVertexCount() const { return m_NumVertices; }
		u32 GetPolygonCount() const { return m_NumPolygons; }
		u32 GetIndexCount() const { return m_NumPolygons * 3; }

		phOctantMap* GetOctantMap() const;
	};

	class phBoundGeometry : public phBoundPolyhedron
	{
		using Materials = u64*;
		using MaterialIndices = u8*;

		Materials		m_Materials;
		u64				m_UnknownF8;
		u64				m_Unknown100;
		u64				m_Unknown108;
		u32				m_Unknown110;
		MaterialIndices m_MaterialIds;	// Polygon to material
		u8				m_NumMaterials;
		u8				m_Unknown121;
		u16				m_Unknown122;
		u32				m_Unknown124;
		u32				m_Unknown128;
		u16				m_Unknown12C;

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

		void PostLoadCompute() override;

		void SetCentroidOffset(const Vec3V& offset) override;
		void ShiftCentroidOffset(const Vec3V& offset) override;

		u8 GetNumMaterials() const override { return m_NumMaterials; }
		phMaterial* GetMaterial(int partIndex) const override;
		void SetMaterial(u64 materialId, int partIndex) override;
		u64 GetMaterialIdFromPartIndex(int partIndex, int boundIndex = -1) const override;

		void CalculateExtents() override;

		// -- Extra virtual functions 

		virtual void CalcCGOffset(Vec3V& outOffset)
		{
			// TODO:
		}

		virtual u64 GetMaterialID(int partIndex);
		virtual u64 GetPolygonMaterialIndex(int polygon);
		virtual void SetPolygonMaterialIndex(int polygon, int materialIndex);
		virtual bool HasOpenEdges()
		{
			// TODO:
			return false;
		}
		virtual ConstString GetMaterialName(int materialIndex);
		virtual void ScaleSize(float x, float y, float z)
		{
			// TODO:
		}

	private:
		// Nullsub
		virtual void FunctionC8() { }
	public:

		void SetMarginAndShrink(float margin = PH_DEFAULT_MARGIN, float t = 0.0f);
		void SetMesh(const spdAABB& bb, const Vector3* vertices, const u16* indices, u32 vertexCount, u32 indexCount);

		// Decompresses all vertices and stores them in non-SIMD vector
		amUniquePtr<Vector3> GetVertices(bool shrunkedVertices = false) const;
		// Converts existing triangle polygons to index array
		amUniquePtr<u16> GetIndices() const;
		// Creates vertex buffer that can be used for rendering
		amUniquePtr<char> ComposeVertexBuffer(const grcVertexFormatInfo& vertexInfo) const;

		// ----- Debugging -----

		// Writes all geometry triangles to given stream in OBJ format
		void WriteObj(const fiStreamPtr& stream, bool shrunkedVertices = false) const;
		void WriteObj(ConstString path, bool shrunkedVertices = false) const;
		// Prints all octant indices & counts
		void PrintOctantMap(bool printIndices = false) const;
	};
}
