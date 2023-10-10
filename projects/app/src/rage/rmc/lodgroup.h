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

	/**
	 * \brief Collection of models belonging to particular drawable lod.
	 */
	class rmcLod
	{
		pgUPtrArray<grmModel> m_Models;

	public:
		rmcLod() = default;
		rmcLod(const datResource& rsc)
		{

		}
		rmcLod(const rmcLod& other) = default;

		pgUPtrArray<grmModel>& GetModels() { return m_Models; }
		grmModel* GetModel(u16 index) { return m_Models[index].Get(); }
	};

	/**
	 * \brief Bounds and LODs definitions.
	 */
	class rmcLodGroup
	{
		spdSphere			m_BoundingSphere;			// Aka culling sphere
		spdAABB				m_BoundingBox;
		pgUPtr<rmcLod>		m_Lods[LOD_COUNT];
		float				m_LodThreshold[LOD_COUNT];	// Max render distance for each lod
		grcDrawBucketMask	m_LodRenderMasks[LOD_COUNT];

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

		void ComputeBucketMask(grmShaderGroup* shaderGroup)
		{

			// TODO: ...
			//for(int i = 0; i < LOD_COUNT; i++)
			//{
			//	m_LodRenderMasks[i] = m_LodModels->Get()[i].ComputeBucketMask(shaderGroup);
			//}
		}

		u32 GetBucketMask(int lod)
		{
			__int64 _lod; // r8
			u32* i; // rcx

			_lod = lod;
			if (lod >= 4i64)
				return 0;
			for (i = (u32*)&this->m_LodRenderMasks[lod];
				!*i;
				++i)
			{
				if (++_lod >= 4)
					return 0;
			}
			return *i & 0xFFFEFFFF;
		}

		void DrawSingle(const grmShaderGroup* shaderGroup, const Mat34V& mtx, grcDrawBucketMask mask, eDrawableLod lod) const
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
