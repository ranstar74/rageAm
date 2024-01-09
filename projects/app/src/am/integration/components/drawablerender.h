#pragma once

#ifdef AM_INTEGRATED

#include "am/integration/gameentity.h"
#include "am/integration/integration.h"
#include "am/integration/updatecomponent.h"
#include "rage/physics/bounds/geometry.h"

namespace rageam::integration
{
	class DrawableRender : public IUpdateComponent
	{
		ComponentOwner<GameEntity> m_Entity;

		void RenderBoneRecurse(rage::crSkeletonData* skel, const rage::crBoneData* rootBone, u32 depth = 0);
		void RenderBound_Geometry(const rage::phBoundGeometry* bound) const;
		void RenderBound(rage::phBound* bound);
		void OnEarlyUpdate() override;
		void OnUiUpdate() override;

		void ReleaseAllRefs() override
		{
			m_Entity.Release();
		}

	public:
		void SetEntity(const ComponentOwner<GameEntity>& gameEntity)
		{
			m_Entity = gameEntity;
		}

		static inline bool BoundingBox = true;
		static inline bool BoundingSphere = false;
		static inline bool Skeleton = true;
		static inline bool Collision = true;
	};
}

#endif
