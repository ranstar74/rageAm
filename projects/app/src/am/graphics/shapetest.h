//
// File: shapetest.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/math/vecv.h"

// https://github.com/sharpdx/SharpDX/blob/master/Source/SharpDX.Mathematics/Collision.cs
// https://iquilezles.org/articles/intersectors

namespace rageam::graphics
{
	struct ShapeHit
	{
		rage::ScalarV	Distance;
		bool			DidHit;
	};

	class ShapeTest
	{
	public:
		static void ClosestPointOnPlane(
			const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
			const rage::Vec3V& planePos, const rage::Vec3V& planeNormal,
			rage::ScalarV& outDistance);

		static bool RayIntersectsSphere(
			const rage::Vec3V& fromPos, const rage::Vec3V& dir,
			const rage::Vec3V& spherePos, const rage::ScalarV& sphereRadius,
			rage::ScalarV* outDistance = nullptr);

		/**
		 * \brief Determines whether there is an intersection between a ray and a triangle.
		 * \return This method tests if the ray intersects either the front or back of the triangle.
		 * If the ray is parallel to the triangle's plane, no intersection is assumed to have
		 * happened. If the intersection of the ray and the triangle is behind the origin of
		 * the ray, no intersection is assumed to have happened. In both cases of assumptions,
		 * this method returns false.
		 */
		static bool RayIntersectsTriangle(
			const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
			const rage::Vec3V& vertex1, const rage::Vec3V& vertex2, const rage::Vec3V& vertex3, float& outDistance);

		static bool RayIntersectsPlane(
			const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
			const rage::Vec3V& planePos, const rage::Vec3V& planeNormal,
			rage::ScalarV* outDistance);

		static bool RayIntersectsPlane(
			const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
			const rage::Vec3V& planePos, const rage::Vec3V& planeNormal,
			rage::Vec3V& outPoint);

		// RayIntersectsDisk
		static bool RayIntersectsCircle(
			const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
			const rage::Vec3V& circlePos, const rage::Vec3V& circleNormal,
			const rage::ScalarV& circleRadius,
			rage::ScalarV* outDistance = nullptr, rage::Vec3V* outPoint = nullptr);

		static bool RayIntersectsCapsule(
			const rage::Vec3V& rayPos, const rage::Vec3V& rayDir,
			const rage::Vec3V& extentFrom, const rage::Vec3V& extentTo,
			const rage::ScalarV& radius,
			rage::ScalarV* outDistance = nullptr);
	};
}
