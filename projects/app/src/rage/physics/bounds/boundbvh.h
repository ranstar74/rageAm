//
// File: boundbvh.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "boundgeometry.h"
#include "optimizedbvh.h"

namespace rage
{
	class phBoundCuller;

	/**
	 * \brief Spatial accelerated bound container.
	 * The BVH is built once and never again, because it is quite expensive operation.
	 * For BVH that can be rebuilt on runtime, see phBoundComposite.
	 * This type is used mostly on static map collision.
	 */
	class phBoundBVH : public phBoundGeometry
	{
		pgUPtr<phOptimizedBvh> m_BVH;
		pgCArray<u16>          m_ActivePolygonIndices; // Used for debug drawing, unused in release
		u16                    m_NumActivePolygons;
		u8					   m_Pad[14];

	public:
		phBoundBVH();
		phBoundBVH(const atArray<Vector3>& vertices, const atArray<phPrimitive>& primitives);
		phBoundBVH(const datResource& rsc);

		// Do not use GetPolygon function! BVH allows all sort of primitives
		phPrimitive& GetPrimitive(int index) const;
		int GetPrimitiveCount() const { return GetPolygonCount(); }

		void BuildBVH();

		phOptimizedBvh* GetBVH() const { return m_BVH.Get(); }

		virtual void CullSpherePolys(phBoundCuller& culler, const Vec3V& sphereCenter, const ScalarV& sphereRadius) const;
		virtual void CullOBBPolys(phBoundCuller& culler, const Mat34V& boxMatrix, const Vec3V& boxHalfExtents) const;
	};
}
