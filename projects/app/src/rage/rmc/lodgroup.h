//
// File: lodgroup.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/grm/model.h"
#include "rage/spd/aabb.h"

namespace rage
{
	// Drawable support only up to 4 LODs, on vehicles _hi model is used as extra lod
	enum eDrawableLod
	{
		LOD_HIGH,
		LOD_MED,
		LOD_LOW,
		LOD_VLOW,
		LOD_COUNT,
	};

	using grmModels = pgUPtrArray<grmModel>;

	/**
	 * \brief Collection of models belonging to particular drawable lod.
	 */
	class rmcLod
	{
		grmModels m_Models;

	public:
		rmcLod() = default;
		rmcLod(const datResource& rsc)
		{

		}
		rmcLod(const rmcLod& other) = default;

		auto GetModels() -> grmModels& { return m_Models; }
		auto GetModels() const -> const grmModels& { return m_Models; }
		auto GetModel(u16 index) -> grmModel* { return m_Models[index].Get(); }

		u32 ComputeBucketMask(const grmShaderGroup* shaderGroup) const;
	};

	/**
	 * \brief Bounds and LODs definitions.
	 */
	class rmcLodGroup
	{
		spdSphere		m_BoundingSphere;			// Aka culling sphere
		spdAABB			m_BoundingBox;
		pgUPtr<rmcLod>	m_Lods[LOD_COUNT];
		float			m_LodThreshold[LOD_COUNT];	// Max render distance for each lod
		grcDrawMask		m_LodRenderMasks[LOD_COUNT];

	public:
		rmcLodGroup();
		rmcLodGroup(const datResource& rsc);
		rmcLodGroup(const rmcLodGroup& other);

		rmcLod* GetLod(int lod);
		int GetLodCount() const;

		void Copy(const rmcLodGroup& from);
		void CalculateExtents();

		const spdAABB& GetBoundingBox() const { return m_BoundingBox; }
		const spdSphere& GetBoundingSphere() const { return m_BoundingSphere; }

		void SetBounds(const spdAABB& bb, const spdSphere& bs)
		{
			m_BoundingBox = bb;
			m_BoundingSphere = bs;
		}

		// Merges lod model bucket masks into single lod mask
		void ComputeBucketMask(const grmShaderGroup* shaderGroup);
		// Sorts all lod model geometries for tessellation (non-tessellated models before tessellated)
		void SortForTessellation(const grmShaderGroup* shaderGroup) const;
		// Gets bucket mask for closest available to given lod
		u32 GetBucketMask(int lod) const;
		void SetRenderMask(u32 mask, int lod) { m_LodRenderMasks[lod] = mask; }

		void DrawSingle(const grmShaderGroup* shaderGroup, const Mat34V& mtx, grcDrawMask mask, eDrawableLod lod) const
		{
			//if (!mask.DoTest(m_LodBucketMasks[lod]))
			//	return;

			pgUPtrArray<grmModel>& models = m_Lods[lod]->GetModels();
			if (!models.Any()) // Early exit because nothing to render
				return;

			grcDevice::SetWorldMtx(mtx);

			for (const pgUPtr<grmModel>& model : models)
			{
				// TODO: Tessellated rendering

				model->DrawUntessellatedPortion(shaderGroup, mask);
			}
		}
	};
}
