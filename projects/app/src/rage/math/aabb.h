//
// File: aabb.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "vec.h"

namespace rage
{
	/**
	 * \brief Non-vectorized version of spdAABB.
	 */
	struct AABB
	{
		Vector3 Min;
		Vector3 Max;

		AABB() { Min = S_MIN; Max = S_MAX; }
		AABB(const Vector3& min, const Vector3& max) { Min = min; Max = max; }
		AABB(const spdAABB& aabb) { Min = aabb.Min, Max = aabb.Max; }
	};
}
