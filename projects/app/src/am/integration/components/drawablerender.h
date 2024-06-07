#pragma once

#ifdef AM_INTEGRATED

#include "am/integration/gameentity.h"
#include "am/graphics/color.h"

namespace rage
{
	class phBoundGeometry;
	class phBoundBVH;
}

namespace rageam::integration
{
	class DrawableRender : public IUpdateComponent
	{
		GameEntity* m_Entity = nullptr;

		void RenderBoneRecurse(rage::crSkeletonData* skel, const rage::crBoneData* rootBone, u32 depth = 0);
		void RenderBound_Geometry(const rage::phBoundGeometry* bound) const;
		void RenderBound_BVH(const rage::phBoundBVH* bound) const;
		void RenderCollisionBound(rage::phBound* bound);
		void OnUpdate() override;

	public:
		void SetEntity(GameEntity* entity) { m_Entity = entity; }

		static inline bool IsVisible = true; // One switch for all debug overlays

		static inline bool DrawBoundsOnTop = true;
		static inline bool BoundingBox = true;
		static inline bool BoundingSphere = false;
		static inline bool Skeleton = true;
		static inline bool BoneTags = true;
		static inline bool Collision = true;
		static inline bool LightNames = false;

		// Bounding box / sphere
		static inline u32 BoundingOutlineColor = graphics::ColorU32(30, 160, 55, 240);
		// Collision mesh
		// TODO: Separate color for mesh, primitives, BVH
		static inline u32 CollisionBoundColor = graphics::ColorU32(201, 201, 201, 81);
	};
}

#endif
