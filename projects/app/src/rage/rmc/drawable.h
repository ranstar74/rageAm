//
// File: drawable.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "lodgroup.h"
#include "rage/creature/jointdata.h"
#include "rage/creature/skeletondata.h"
#include "rage/grm/shadergroup.h"
#include "rage/paging/base.h"
#include "rage/paging/ref.h"

namespace rage
{
	/**
	 * \brief Drawable represents 3D scene the same way as .fbx or .obj files do.
	 */
	class rmcDrawable : public pgBase
	{
	protected:
		pgUPtr<grmShaderGroup>	m_ShaderGroup;
		pgPtr<crSkeletonData>	m_SkeletonData;
		rmcLodGroup				m_LodGroup;
		pgPtr<crJointData>		m_JointData;
		u64						m_Unknown98;
		u64						m_UnknownA0;
		u64						m_UnknownA8;

		void CopyShaderGroupToContainer(const grmShaderGroup* from);
		void DeleteShaderGroupContainer();

	public:
		rmcDrawable();
		rmcDrawable(const datResource& rsc);
		rmcDrawable(const rmcDrawable& other);
		~rmcDrawable() override;

		crSkeletonData* GetSkeletonData() const { return m_SkeletonData.Get(); }
		void SetSkeletonData(crSkeletonData* skeleton) { m_SkeletonData = skeleton; }
		rmcLodGroup& GetLodGroup() { return m_LodGroup; }
		grmShaderGroup* GetShaderGroup() const { return m_ShaderGroup.Get(); }

		virtual void Delete();

		virtual void Draw(const Mat34V& mtx, grcDrawBucketMask mask, eDrawableLod lod);
		virtual void DrawSkinned(const Mat34V& mtx, u64 mtxSet, grcDrawBucketMask mask, eDrawableLod lod);
		virtual void DrawNoShaders() {}
		virtual void DrawSkinnedNoShaders() {}
		virtual void IsVisible34() {}
		virtual void IsVisible44() {}
		virtual u32 GetBucketMask(int lod);
	};
}
