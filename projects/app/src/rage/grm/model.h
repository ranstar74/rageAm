//
// File: model.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "geometry.h"
#include "shadergroup.h"
#include "rage/dat/base.h"
#include "rage/spd/aabb.h"

namespace rage
{
	/**
	 * \brief When model contains multiple geometries, AABB's array will contain additional combined BB,
	 * which is used to quickly tell if any of multiple geometries is visible in viewport at all.
	 */
	struct grmAABBGroup
	{
		union
		{
			struct Combined
			{
				spdAABB AABB; // Combined bounding box
				spdAABB AABBs[1];
			} Combined;

			struct Single
			{
				spdAABB AABBs[1];
			} Single;
		};

		spdAABB* GetAABBs(u16 bbCount);
		spdAABB& GetCombinedAABB(u16 bbCount);

		// Must be called once we're done with editing group
		void ComputeCombinedBB(u16 bbCount);

		static u16 GetIncludingCombined(u16 bbCount);
		static bool HasCombinedBB(u16 bbCount);

		static void Add(pgUPtr<grmAABBGroup>& group, u16 bbCount, const spdAABB& bbToAdd);
		static void Remove(const pgUPtr<grmAABBGroup>& group, u16 bbCount, u16 index);

		static pgUPtr<grmAABBGroup> Allocate(u16 bbCount);
		static pgUPtr<grmAABBGroup> Copy(const pgUPtr<grmAABBGroup>& other, u16 bbCount);
	};

	using grmGeometries = pgUPtrArray<grmGeometryQB>;

	/**
	 * \brief Also known as 'Mesh'. Model, just like mesh, may contain multiple primitives (Geometries).
	 */
	class grmModel : public datBase
	{
		// NOTE: Native code uses names 'MatrixCount' 'MatrixIndex' for bones but I've decided that bone is simpler to understand

		grmGeometries			m_Geometries;
		// For multiple bounding boxes there's the main one for all of them (basically model BB)
		pgUPtr<grmAABBGroup>	m_AABBs;
		pgCArray<u16>			m_GeometryToMaterial;
		u8						m_BoneCount; // Num of bones linked to this skinned model
		// 1 -	Relative transforms for skinning. Used on all models that use actual bone skinning (with bone weights)
		//		If you don't set this, game will apply default bone transformations to the bones and they will appear offset
		u8						m_Flags;
		u8						m_Type;
		// Used for 'transform' skinning without weighting, which basically links model to bone without deformation
		u8						m_BoneIndex;
		u8						m_ModelMask; // Sub draw buckets (RB_MODEL_###)
		// IsSkinned - the lowest bit, the rest 7 high bits are number of geometries with tessellation
		u8						m_TesselatedCountAndIsSkinned;
		// Not sure why this variable exists in the first place because m_Geometries is atArray,
		// but we must keep and maintain it anyway because game & other tools use it
		// This possibly could be num of 'active' geometries
		u16						m_GeometryCount;
		u64						m_Unknown30;

		void SetTesselatedGeometryCount(u8 count)
		{
			m_TesselatedCountAndIsSkinned &= 1;
			if (count > 0) m_TesselatedCountAndIsSkinned |= count << 1;
		}

	public:
		grmModel();

		// ReSharper disable once CppPossiblyUninitializedMember
		grmModel(const datResource& rsc) : m_Geometries(rsc)
		{

		}

		grmModel(const grmModel& other);

		const spdAABB& GetCombinedBoundingBox() const;
		const spdAABB& GetGeometryBoundingBox(u16 geometryIndex) const;

		const grmGeometries& GetGeometries() { return m_Geometries; }

		u16 GetMaterialIndex(u16 geometryIndex) const;
		void SetMaterialIndex(u16 geometryIndex, u16 materialIndex) const;

		void SetIsSkinned(bool skinned, u8 numBones = 0);
		bool IsSkinned() const { return m_TesselatedCountAndIsSkinned & 1; }
		u8 GetTesselatedGeometryCount() const { return m_TesselatedCountAndIsSkinned >> 1; }

		// Must be called once tessellation was added / removed, recomputes RB_MODEL_TESSELLATION in draw mask
		void UpdateTessellationDrawBucket();

		u32  GetSubDrawBucketMask() const { return m_ModelMask << RB_NUM_BASE_BUCKETS; }
		void SetSubDrawBucketMask(u32 mask) { m_ModelMask = u8((mask >> RB_NUM_BASE_BUCKETS) & 0xFF); }
		// RB_MODEL_
		void SetSubDrawBucketFlags(u32 flags, bool on);
		bool IsSubDrawBucketFlagSet(grcRenderBucketTypes flag) const { return GetSubDrawBucketMask() & (1 << flag); }

		u16 GetGeometryCount() const { return m_GeometryCount; }

		u16 GetBoneIndex() const { return m_BoneIndex; }
		void SetBoneIndex(u16 index) { m_BoneIndex = index; }

		// Recomputes combined bounding box, must be called after all geometries are set
		void ComputeAABB() const;
		void TransformAABB(const Mat44V& mtx) const;

		// Note: Ownership of geometry pointer is moved to grmModel! It will be swapped to nullptr.
		pgUPtr<grmGeometryQB>& AddGeometry(pgUPtr<grmGeometryQB>& geometry, const spdAABB& bb);
		void RemoveGeometry(u16 index);

		// Sorts model geometries to group tessellated geometries after regular ones,
		// this allows to render faster by batching all tessellated/non models at once.
		// The function must be called before rendering:
		// - Any time the geometry list has been changed
		// - Geometry material changed from/to tessellated
		void SortForTessellation(const grmShaderGroup* shaderGroup);

		u32 ComputeBucketMask(const grmShaderGroup* shaderGroup) const;

		// Use unused flag bit for outline flag
		bool HasOutline() const { return m_Flags & 0x80; }
		void SetOutline(bool b)
		{
			m_Flags &= ~0x80;
			if (b) m_Flags |= 0x80;
		}

		// -- Draw functions --

		void DrawPortion(int a2, u32 startGeometryIndex, u32 totalGeometries, const grmShaderGroup* shaderGroup, grcDrawMask mask) const;
		void DrawUntessellatedPortion(const grmShaderGroup* shaderGroup, grcDrawMask mask) const;
		void DrawTesellatedPortion(const grmShaderGroup* shaderGroup, grcDrawMask mask, bool a5) const;
	};
}
