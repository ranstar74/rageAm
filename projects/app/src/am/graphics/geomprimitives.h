//
// File: geomprimitives.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/types.h"
#include "rage/math/mathv.h"

namespace rageam::graphics
{
	enum PrimitiveType
	{
		PrimitiveInvalid,
		PrimitiveMesh, // Not really a primitive, only for API convinience
		PrimitiveBox,
		PrimitiveSphere,
		PrimitiveCylinder,
		PrimitiveCapsule,
	};

	struct Primitive
	{
		PrimitiveType Type;
		AABB		  AABB;
		union
		{
			// Triangle topology
			struct
			{
				// Those are pointers to original mesh data passed to the MatchPrimitive function
				Vec3S* Points;
				u16*   Indices;
				int    PointCount;
				int    IndexCount;
			} Mesh;
			// Oriented Bounding Box
			struct
			{
				Vec3V Points[4]; // Coordinates of the OBB diagonals
			} Box;
			struct
			{
				Vec3V   Center;
				ScalarV Radius;
			} Sphere;
			struct
			{
				Vec3V   Center;
				Vec3V   Direction;
				ScalarV Radius;
				ScalarV HalfHeight;
			} Cylinder, Capsule;
		};

		Primitive()
		{
			Type = PrimitiveInvalid;
			Capsule = {}; // Supress compiler warnings
		}

		Primitive& operator=(const Primitive& other)
		{
			Type = other.Type;

			// Copy the largest type
			memcpy(&Capsule, &other.Capsule, sizeof Capsule); // NOLINT(bugprone-undefined-memory-manipulation)

			return *this;
		}
	};

	// Does not account closed/open edges
	inline float ComputeMeshVolume(const Vec3S* points, const u16* indices, int indexCount)
	{
		ScalarV volume = 0.0f;

		int triCount = indexCount / 3;
		for (int i = 0; i < triCount; i++)
		{
			Vec3V p1 = points[indices[i * 3 + 0]];
			Vec3V p2 = points[indices[i * 3 + 1]];
			Vec3V p3 = points[indices[i * 3 + 2]];
			
			volume += p1.Dot(p2.Cross(p3)) / 6.0f;
		}

		return volume.Get();
	}

	// Attempts to match given points in 3D space to a geometric primitive
	// NOTE:
	// - Unusual topology is not recognized!
	// - AABB is not used by algorithm but set to Primitive::AABB
	inline bool MatchPrimitive(const AABB& bb, Vec3S* points, int pointCount, u16* indices, int indexCount, Primitive& outPrimitive)
	{
		// Initialize default primitive type
		outPrimitive.Type = PrimitiveMesh;
		outPrimitive.AABB = bb;
		outPrimitive.Mesh.Points = points;
		outPrimitive.Mesh.Indices = indices;
		outPrimitive.Mesh.PointCount = pointCount;
		outPrimitive.Mesh.IndexCount = indexCount;

		// This always should be true, just sanity check to prevent overrun of stack allocated arrays below
		AM_ASSERTS(indexCount >= pointCount);
		AM_ASSERTS(indexCount % 3 == 0); // Make sure that we're dealing with triangles

		int polyCount = indexCount / 3;

		// Sphere has the highest polygon count: Blender & 3DS Max create sphere with 960 tris
		constexpr int	MAX_PRIMITIVE_TRIS = 4096;
		constexpr int	MAX_PRIMITIVE_INDICES = MAX_PRIMITIVE_TRIS * 3;
		constexpr float PRIMITIVE_MAX_ERROR = 0.01f;  // Maximum allowed error (deviation) from expected value
		constexpr float PRIMITIVE_MAX_DISTSQ = 0.01f; // Maximum allowed squared deviation from expected position
		// Volume can be quite off because of polygon imperfections, allow higher error here
		constexpr float PRIMITIVE_VOLUME_MAX_ERROR = 0.9f;
		if (indexCount == 0 || indexCount > MAX_PRIMITIVE_INDICES)
			return false;

		auto almostEquals =		  [](float a, float b) { return rage::AlmostEquals(a, b, PRIMITIVE_MAX_ERROR); };
		auto almostEqualsDist =   [](float a, float b) { return rage::AlmostEquals(a, b, PRIMITIVE_MAX_DISTSQ); };
		auto almostEqualsVolume = [](float a, float b) { return rage::AlmostEquals(a, b, PRIMITIVE_VOLUME_MAX_ERROR); };

		auto getTriNormal = [&](u16 triIndex)
			{
				Vec3V v1 = points[indices[triIndex * 3 + 0]];
				Vec3V v2 = points[indices[triIndex * 3 + 1]];
				Vec3V v3 = points[indices[triIndex * 3 + 2]];
				return GetTriNormal(v1, v2, v3);
			};

		auto getTriHypotenuse = [&](u16 triIndex, Vec3V& outV1, Vec3V& outV2)
			{
				Vec3V v1 = points[indices[triIndex * 3 + 0]];
				Vec3V v2 = points[indices[triIndex * 3 + 1]];
				Vec3V v3 = points[indices[triIndex * 3 + 2]];
				GetTriHypotenuse(v1, v2, v3, outV1, outV2);
			};

		Vec3V center;
		for (int i = 0; i < pointCount; i++)
			center += Vec3V(points[i]);
		center /= static_cast<float>(pointCount);

		// Box: 6 faces, 12 triangles, 36 indices
		if (indexCount == 36)
		{
			// Indices of two opposite polygons
			int polygon1 = -1;
			int polygon2 = -1;

			// Angle between box normals is either 0°, 90° or 180°
			Vec3V refNormal = getTriNormal(0);
			int num0 = 0;
			int num90 = 0;
			int num180 = 0;
			for (int i = 1; i < 12; i++)
			{
				Vec3V normal = getTriNormal(i);
				float dot = refNormal.Dot(normal).Get();
				if (almostEquals(dot, 1.0f))
				{
					num0++;
					polygon1 = i;
				}
				else if (almostEquals(dot, 0.0f))
				{
					num90++;
				}
				else if (almostEquals(dot, -1.0f))
				{
					num180++;
					polygon2 = i;
				}
				else
					break; // Not a box...
			}

			// 0°   - Adjacent polygon on the same face
			// 90°  - All except opposide face (4 faces)
			// 180° - Opposite face
			if (num0 == 1 && num90 == 8 && num180 == 2)
			{
				Vec3V vert0, vert1, vert2, vert3;
				// Take diagonals from opposite polygons
				// NOTE: This expects correct box face orientation!
				getTriHypotenuse(polygon1, vert0, vert1);
				getTriHypotenuse(polygon2, vert2, vert3);

				bool isValidDiagonals = true;
				if (vert0.DistanceToSquared(vert1) != vert2.DistanceToSquared(vert3))
					isValidDiagonals = false; // Diagonals arent the same length
				else if (vert0.DistanceToSquared(vert3) != vert1.DistanceToSquared(vert2))
					isValidDiagonals = false; // Diagonals are not parallel

				if (isValidDiagonals)
				{
					outPrimitive.Type = PrimitiveBox;
					outPrimitive.Box.Points[0] = vert0;
					outPrimitive.Box.Points[1] = vert1;
					outPrimitive.Box.Points[2] = vert2;
					outPrimitive.Box.Points[3] = vert3;
					return true;
				}
			}
		}

		// Not enough polygons to form any other shape (80 is ico sphere tri count in blender)
		if (indexCount < 80)
			return false;

		// And distances from the center
		auto  distancesFromCenterSq = std::unique_ptr<float[]>(new float[pointCount]);
		float minDistanceFromCenterSq = INFINITY;
		float maxDistanceFromCenterSq = -INFINITY;
		int   minDistanceFromCenterIndex = -1; // Closest point to the center
		int   maxDistanceFromCenterIndex = -1; // Most distant point from the center
		bool  allDistancesAreEqual = true;	   // Extra check for sphere test
		for (int i = 0; i < pointCount; i++)
		{
			float distanceSq = Vec3V(points[i]).DistanceToSquared(center).Get();
			distancesFromCenterSq[i] = distanceSq;
			// Can't compare first vertex with anything
			if (allDistancesAreEqual && i > 0 && !almostEqualsDist(distancesFromCenterSq[i - 1], distanceSq))
				allDistancesAreEqual = false;

			if (distanceSq < minDistanceFromCenterSq)
			{
				minDistanceFromCenterSq = distanceSq;
				minDistanceFromCenterIndex = i;
			}

			if (distanceSq > maxDistanceFromCenterSq)
			{
				maxDistanceFromCenterSq = distanceSq;
				maxDistanceFromCenterIndex = i;
			}
		}

		// Sphere: Distance between all of the points must be equal
		if (allDistancesAreEqual)
		{
			// Ensure that shape is indeed a sphere by comparing volume, this will cull
			// half spheres, planes, cylinders, and other shapes with all points equally distanced from the center
			float radius = sqrtf(distancesFromCenterSq[0]);
			float volume = ComputeMeshVolume(points, indices, indexCount);
			float expectedVolume = 4.0f / 3.0f * rage::PI * powf(radius, 3);
			if (almostEqualsVolume(volume, expectedVolume))
			{
				outPrimitive.Type = PrimitiveSphere;
				outPrimitive.Sphere.Center = center;
				outPrimitive.Sphere.Radius = radius;
				return true;
			}
		}

		// Cylinder: We have to find normal of bottom or top face,
		// we know for sure that only top & bottom faces share many vertices on
		// the same plane, while on cylinder body there are 2 triangles per plane
		Vec3V cylinderNormal;
		bool  cylinderIsValid = false;
		for (int i = 0; i < polyCount; i++)
		{
			Vec3V normal = getTriNormal(i);
			// Find at least 3 triangles on the same plane
			int matchCount = 0;
			for (int j = 0; j < polyCount; j++)
			{
				if (i == j)
					continue;

				// Check if we found polygon with the same normal
				Vec3V otherNormal = getTriNormal(j);
				if (!almostEquals(normal.Dot(otherNormal).Get(), 1.0f))
					continue;

				// Check if we found more than 2 polygons to make sure we're on top or bottom face
				matchCount++;
				if (matchCount > 2)
					break;
			}

			// We found triangle on bottom or top face
			if (matchCount > 2)
			{
				cylinderNormal = normal;
				cylinderIsValid = true;
				break;
			}
		}
		if (cylinderIsValid)
		{
			// Now make sure that:
			// 1) All points are equally distanced from the center (forming a circle)
			//		except for the mid point (if present)
			// 2) Top and bottom faces have the same vertex count
			int vertexCount1 = 0;
			int vertexCount2 = 0;

			float prevDistanceSq = -1.0f;

			// Store it to later find cylinder radius
			int nonCenterPointIndex = -1;

			for (int j = 0; j < pointCount; j++)
			{
				// Skip point in the center of the face (it might not exist at all, depending on topology)
				Vec3V toPoint = Vec3V(points[j] - center).NormalizedEstimate();
				float toPointDot = cylinderNormal.Dot(toPoint).Get();
				if (almostEquals(toPointDot, 1.0f) || almostEquals(toPointDot, -1.0f))
					continue;

				nonCenterPointIndex = j;

				float distanceSq = distancesFromCenterSq[j];
				// We are not dealing with the cylinder, skip
				if (prevDistanceSq >= 0.0f && !almostEqualsDist(distanceSq, prevDistanceSq))
				{
					cylinderIsValid = false;
					break;
				}

				// Depending on the angle between normal we either dealing with top or bottom face
				if (cylinderNormal.Dot(toPoint).Get() >= 0.0f)
					vertexCount1++;
				else
					vertexCount2++;

				prevDistanceSq = distanceSq;
			}

			// If still valid, because it might have failed on circle equality check
			if (cylinderIsValid && vertexCount1 == vertexCount2)
			{
				// Project random point on the cylinder normal (it's gonna be any point on top or bottom plane)
				// on the normal to get point exactly in the center, we'll get radius this way)
				Vec3V point = points[nonCenterPointIndex] - center;
				Vec3V projectedPoint = point.Project(cylinderNormal);
				float halfHeight = center.DistanceTo(projectedPoint).Get();
				float radius = point.DistanceTo(projectedPoint).Get();

				float volumeExpected = rage::PI * radius * radius * halfHeight * 2;
				float volume = ComputeMeshVolume(points, indices, indexCount);
				if (almostEqualsVolume(volume, volumeExpected))
				{
					outPrimitive.Type = PrimitiveCylinder;
					outPrimitive.Cylinder.Center = center;
					outPrimitive.Cylinder.Radius = radius;;
					outPrimitive.Cylinder.HalfHeight = halfHeight;
					outPrimitive.Cylinder.Direction = cylinderNormal;
					return true;
				}
			}
		}

		// Capsule: Use the most distant point from the center (top of the half sphere) to get
		// direction, make sure that half-sphere is indeed half-sphere... by comparing radius
		Vec3V   capsuleTop = points[maxDistanceFromCenterIndex];
		Vec3V   capsuleToTop = capsuleTop - center;
		Vec3V   capsuleNormal = capsuleToTop.Normalized();
		// Those are 'centres' of capsule half spheres, placed as if they were full spheres
		Vec3V   capsuleToHalfSphereBase1 = Vec3V(points[minDistanceFromCenterIndex] - center).Project(capsuleNormal);
		Vec3V   capsuleToHalfSphereBase2 = center - capsuleToHalfSphereBase1;
		ScalarV capsuleHeight = capsuleToHalfSphereBase1.DistanceTo(capsuleToHalfSphereBase2);
		ScalarV capsuleRadiusSq = capsuleToHalfSphereBase1.DistanceToSquared(capsuleToTop);
		bool    capsuleIsValid = true;
		for (int i = 0; i < pointCount; i++)
		{
			Vec3V   point = points[i] - center;
			ScalarV radiusSq;
			if (point.Dot(capsuleNormal) >= 0.0f)
				radiusSq = point.DistanceToSquared(capsuleToHalfSphereBase1);
			else
				radiusSq = point.DistanceToSquared(capsuleToHalfSphereBase2);

			// We are not dealing with capsule, this is not a half sphere
			if (!almostEqualsDist(radiusSq.Get(), capsuleRadiusSq.Get()))
			{
				capsuleIsValid = false;
				break;
			}
		}
		if (capsuleIsValid)
		{
			// Make sure that we don't have holes in our capsule by comparing volume
			float radius = capsuleRadiusSq.Sqrt().Get();
			float height = capsuleHeight.Get();
			float volumeExpected = rage::PI * radius * radius * (4.0f / 3.0f * radius + height);
			float volume = ComputeMeshVolume(points, indices, indexCount);
			if (almostEqualsVolume(volume, volumeExpected))
			{
				outPrimitive.Type = PrimitiveCapsule;
				outPrimitive.Capsule.Center = center;
				outPrimitive.Capsule.Radius = radius;
				outPrimitive.Capsule.HalfHeight = height * 0.5f;
				outPrimitive.Capsule.Direction = capsuleNormal;
				return true;
			}
		}

		return false;
	}
}
