//
// File: box.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "primitive.h"

namespace rage
{
	/**
	 * \brief Axis-Aligned Box Collider.
	 */
	class phBoundBox : public phBoundPrimitive
	{
	public:
		phBoundBox(const Vec3V& size = S_ONE);
		phBoundBox(const Vec3V& min, const Vec3V& max);
		phBoundBox(const spdAABB& aabb) : phBoundBox(aabb.Min, aabb.Max) {}
		phBoundBox(const datResource& rsc) : phBoundPrimitive(rsc) {}

		void CalculateExtents() override;
	};
}
