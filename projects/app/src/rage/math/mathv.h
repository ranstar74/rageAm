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

	inline ScalarV GetTriArea(const Vec3V& v1, const Vec3V& v2, const Vec3V& v3)
	{
		Vec3V cross = GetTriCross(v1, v2, v3);
		return cross.Length() * S_HALF;
	}

	inline void GetTriHypotenuse (const Vec3V& v1, const Vec3V& v2, const Vec3V& v3, Vec3V& outV1, Vec3V& outV2)
	{
		Vec3V e1 = v1 - v2;
		Vec3V e2 = v1 - v3;
		Vec3V e3 = v2 - v3;
		ScalarV l1 = e1.LengthSquared();
		ScalarV l2 = e2.LengthSquared();
		ScalarV l3 = e3.LengthSquared();
		if (l1 > l2 && l1 > l3)
		{
			outV1 = v1;
			outV2 = v2;
			return;
		}
		if (l2 > l3)
		{
			outV1 = v1;
			outV2 = v3;
			return;
		}
		outV1 = v2;
		outV2 = v3;
	};

	inline Vec3V ProjectOnPlane(const Vec3V& point, const Vec3V& planePos, const Vec3V& planeNormal)
	{
		Vec3V pointRelativeToPlane = point - planePos;
		Vec3V offsetToPlane = pointRelativeToPlane.Project(planeNormal);
		return point - offsetToPlane;
	}

	// Closest is not at the same position!
	// Templated to support Vec3V & Vector3
	template<typename TVec>
	void FindClosestAndMostDistantPoints(const TVec* points, int pointCount, const Vec3V& from, Vec3V* outClosest, Vec3V* outDistant)
	{
		Vec3V min = from;
		Vec3V max = from;
		ScalarV maxDistSq = S_MIN;
		ScalarV minDistSq = S_MAX;

		for (int i = 0; i < pointCount; i++)
		{
			Vec3V point = points[i];
			ScalarV dist = from.DistanceToSquared(point);

			if (dist > maxDistSq)
			{
				maxDistSq = dist;
				max = point;
			}

			if (!dist.AlmostEqual(S_ZERO) && dist < minDistSq)
			{
				minDistSq = dist;
				min = point;
			}
		}

		if (outClosest) *outClosest = min;
		if (outDistant) *outDistant = max;
	};
}
