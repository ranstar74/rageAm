#pragma once

#ifdef AM_INTEGRATED

#include "am/integration/gameentity.h"
#include "rage/physics/bounds/geometry.h"
#include "am/graphics/color.h"

namespace rageam::integration
{
	class DrawableRender : public IUpdateComponent
	{
		GameEntity* m_Entity = nullptr;

		void RenderBoneRecurse(rage::crSkeletonData* skel, const rage::crBoneData* rootBone, u32 depth = 0);
		void RenderBound_Geometry(const rage::phBoundGeometry* bound) const;
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

		// Bounding box / sphere
		static inline u32 BoundingOutlineColor = graphics::ColorU32(30, 160, 55, 240);
		// Collision mesh
		static inline u32 CollisionBoundColor = graphics::ColorU32(201, 201, 201, 81);
	};
}

#endif
