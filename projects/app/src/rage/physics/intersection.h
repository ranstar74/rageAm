//
// File: intersection.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/math/vecv.h"

namespace rage
{
	// https://github.com/sharpdx/SharpDX/blob/master/Source/SharpDX.Mathematics/Collision.cs

	/**
	 * \brief Determines whether there is an intersection between a ray and a triangle.
	 * \return This method tests if the ray intersects either the front or back of the triangle.
	 * If the ray is parallel to the triangle's plane, no intersection is assumed to have
	 * happened. If the intersection of the ray and the triangle is behind the origin of
	 * the ray, no intersection is assumed to have happened. In both cases of assumptions,
	 * this method returns false.
	 */
	bool RayIntersectsTriangle(
		const Vec3V& rayPos, const Vec3V& rayDir,
		const Vec3V& vertex1, const Vec3V& vertex2, const Vec3V& vertex3, float& outDistance);
}
