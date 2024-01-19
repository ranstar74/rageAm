//
// File: geometry.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/integration/memory/address.h"
#include "rage/grcore/buffer.h"
#include "rage/grcore/device.h"
#include "rage/paging/ref.h"
#include "rage/paging/resource.h"
#include "rage/spd/aabb.h"

namespace rage
{
#if APP_BUILD_2699_16_RELEASE_NO_OPT
	class mshMesh;
	class GeometryCreateParams;
#endif

	struct grmVertexData
	{
		pVoid				Vertices;
		grcIndices_t		Indices;
		u32					VertexCount;
		u32					IndexCount;
		grcVertexFormatInfo	Info;
	};

	/**
	 * \brief Matches rage::grcDrawMode.
	 */
	enum grcPrimitiveType : u8
	{
		PRIM_POINT,
		PRIM_LINES,
		PRIM_LINE,
		PRIM_TRIANGLES,
		PRIM_TRIANGLE,
	};

	/**
	 * \brief Drawable primitive (array of polygons, lines).
	 * \remarks
	 * Geometry material is defined by grmModel::m_GeometryToMaterial (one material per geometry).
	 */
	class grmGeometryQB
	{
		// Vertex & Index buffers are split on 4 areas (QB is apparently Quad ###)
		// This system is totally un-researched and barely used even in native models
		// Based on info from rdr1 remaster, there's 'optimized' vertex buffer used as shadow model? Need more research
		static constexpr int GRID_SIZE = 4;

		grcVertexDeclarationPtr			m_VertexDeclaration;
		s32								m_Unknown10;
		s32								m_Unknown14;
		pgUPtr<grcVertexBufferD3D11>	m_VertexBuffers[GRID_SIZE];
		pgUPtr<grcIndexBufferD3D11>		m_IndexBuffers[GRID_SIZE];
		u32								m_IndexCount;				// Num of indices that will be rendered (Passed to DrawIndexed)
		u32								m_PrimitiveCount;			// Unused
		u16								m_VertexCount;				// Unused, also uses 16 bits for some reason
		grcPrimitiveType				m_PrimitiveType;
		u8								m_QuadTreeData;
		pgUPtr<u16>						m_BoneIDs;
		u8								m_VertexStride;
		u8								m_Unknown71;
		u16								m_BoneIDsCount;
		u64								m_Unused78;
		grcVertexDeclarationPtr			m_UnusedVertexDeclaration;
		pgUPtr<grcVertexBufferD3D11>	m_UnusedVertexBuffer;

		grcVertexBufferD3D11* GetPrimaryVertexBuffer() const;
		grcIndexBufferD3D11* GetPrimaryIndexBuffer() const;

	public:
		grmGeometryQB();
		grmGeometryQB(const datResource& rsc);
		grmGeometryQB(const grmGeometryQB& other) = default;

		void SetVertexData(const grmVertexData& vertexData);

		virtual ~grmGeometryQB() = default;

#if APP_BUILD_2699_16_RELEASE_NO_OPT
		virtual bool Init(const mshMesh& mesh, int mtlIndex, u32 channelMask, const bool isCharClothMesh, const bool isEnvClothMesh, GeometryCreateParams* params = NULL, int extraVerts = 0) { return false; }
#endif

		virtual void Draw();
		virtual void DrawInstanced(u64 instanceData) {}
		virtual void DrawSimple() {}
		virtual void Prefetch() {}
		virtual void DrawSkinned(u64 bones)
		{
			static gmAddress vft = gmAddress::Scan("48 8D 05 ?? ?? ?? ?? 33 F6 48 8B D9 48 89 01 8B 05").GetRef(3);
			static gmAddress drawSkinned = *(u64*)(vft + 0x28);
			static auto fn = drawSkinned.ToFunc<void(grmGeometryQB*, u64)>();
			fn(this, bones);
		}

		virtual const grcFvf* GetFvf() const { return m_VertexBuffers[0]->GetVertexFormat(); }
		virtual const u16* GetBoneIDs() const { return m_BoneIDs.Get(); }
		virtual u16 GetBoneIDsCount() const { return m_BoneIDsCount; }
		virtual void Function0() {}
		virtual bool Function1() { return false; }
		virtual void Function2() {}
		virtual void Function3() {}
		virtual void Function4() {}
		virtual void Function5() {}
		virtual grcVertexBuffer* GetVertexBuffer(int quad) { return m_VertexBuffers[quad].Get(); }
		virtual void Function7() {} // ReplaceVertexBuffers
		virtual void SetVertexBuffer() {}
		virtual void Function9() {}
		virtual void Function10() {}
		virtual void Function11() {}
		virtual void Function12() {}
		virtual grcIndexBuffer* GetIndexBuffer(int quad) const;
		virtual grcPrimitiveType GetPrimitiveType() const { return m_PrimitiveType; }
		virtual u32 GetPrimitiveCount() const { return m_PrimitiveCount; }
		virtual void Function13() {}
		virtual u32 GetVertexCount() const { return m_VertexCount; }
		virtual u32 GetTotalVertexCount() const;
		virtual u32 GetIndexCount() const { return m_IndexCount; }
		virtual void Function14() {}
		virtual void Function15() {}
		virtual void Clone() {}
		virtual void Function16() {}
		virtual void Function17() {}
		virtual void Function18() {}
		virtual void Function19() {}
		virtual void Function20() {}
	};
	using grmGeometry = grmGeometryQB;
}
