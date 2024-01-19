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
#if APP_BUILD_2699_16_RELEASE_NO_OPT
	class rmcTypeFileParser;
	class rmcTypeFileCbData;
	class fiTokenizer;
#endif

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

		const pgPtr<crSkeletonData>& GetSkeletonData() const { return m_SkeletonData; }
		void SetSkeletonData(const pgPtr<crSkeletonData>& skeleton) { m_SkeletonData = skeleton; }
		rmcLodGroup& GetLodGroup() { return m_LodGroup; }
		grmShaderGroup* GetShaderGroup() const { return m_ShaderGroup.Get(); }

		const auto& GetBoundingBox() const { return m_LodGroup.GetBoundingBox(); }
		const auto& GetBoundingSphere() const { return m_LodGroup.GetBoundingSphere(); }

		// After 'usetessellation' was altered in shader params this function must be
		// called in order to update render masks & sort geometries
		// Otherwise tessellation won't be applied
		void ComputeTessellationForShader(u16 shaderIndex);
		// Recomputes bucket mask from draw buckets specified in this drawable shader group,
		// must be called if shader bucket was changed
		void ComputeBucketMask();
		// Sorts all lod model geometries for tessellation (non-tessellated models before tessellated)
		void SortForTessellation() const;

		virtual void Delete();

#if APP_BUILD_2699_16_RELEASE_NO_OPT
		virtual bool Load(const char* basename, rmcTypeFileParser* parser = 0, bool configParser = true) { return false; }
		// Internal helper function, should not be called directly by new code.
		virtual bool Load(fiTokenizer& T, rmcTypeFileParser* parser = 0, bool configParser = true) { return false; }
		// Internal helper function, should not be called directly by new code.
		virtual bool LoadFromParser(fiTokenizer& T, rmcTypeFileParser* parser = 0, bool configParser = true) { return false; }
#endif

		virtual void Draw(const Mat34V& mtx, grcRenderMask mask, eDrawableLod lod);
		virtual void DrawSkinned(const Mat34V& mtx, u64 mtxSet, grcRenderMask mask, eDrawableLod lod);
		virtual void DrawNoShaders() {}
		virtual void DrawSkinnedNoShaders() {}
		virtual void IsVisible34() {}
		virtual void IsVisible44() {}
		virtual u32 GetBucketMask(int lod);

		// Those are defined in different place because original
		// implementation has Load + LoadFromParser functions defined in rmcDrawableBase
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		virtual void LoadMesh(rmcTypeFileCbData* data) {}
		virtual void LoadSkel(rmcTypeFileCbData* data) {}
		virtual void LoadSkelProperties(rmcTypeFileCbData* data) {}
		virtual void LoadJointLimits(rmcTypeFileCbData* data) {}
		virtual void LoadShader(rmcTypeFileCbData* data) {}
		virtual void LoadEdgeModel(rmcTypeFileCbData* data) {}
#endif
	};
}
