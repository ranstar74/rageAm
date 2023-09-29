//
// File: mathv.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "vecv.h"

namespace rage
{
	// Computes cross product of triangle edges (triangle must be winded counter clock wise)
	inline Vec3V GetTriCross(const Vec3V& v1, const Vec3V& v2, const Vec3V& v3)
	{
		Vec3V edge1 = v3 - v2;
		Vec3V edge2 = v1 - v2;
		Vec3V normal = edge1.Cross(edge2);
		return normal;
	}

	// Computes normal of triangle using cross product
	inline Vec3V GetTriNormal(const Vec3V& v1, const Vec3V& v2, const Vec3V& v3)
	{
		Vec3V normal = GetTriCross(v1, v2, v3);
		normal.Normalize();
		return normal;
	}

	// Computes normal of triangle using cross product and estimate reciprocal for normalizing
	inline Vec3V GetTriNormalEstimate(const Vec3V& v1, const Vec3V& v2, const Vec3V& v3)
	{
		Vec3V normal = GetTriCross(v1, v2, v3);
		normal.NormalizeEstimate();
		return normal;
	}
}
