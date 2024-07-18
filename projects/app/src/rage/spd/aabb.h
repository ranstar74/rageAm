//
// File: aabb.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "sphere.h"
#include "rage/math/mtxv.h"
#include "rage/math/vecv.h"
#include "rage/math/vec.h"

namespace rage
{
	/**
	 * \brief Axis-aligned bounding box.
	 */
	struct spdAABB
	{
		Vec3V Min;
		Vec3V Max;

		static spdAABB Empty() { return spdAABB(S_ZERO, S_ZERO); }
		static spdAABB Infinite() { return spdAABB(S_MIN, S_MAX); }

		Vec3V Center() const { return (Min + Max) * S_HALF; }
		ScalarV Width() const { return (Max - Min).X(); }
		ScalarV Length() const { return (Max - Min).Y(); }
		ScalarV Height() const { return (Max - Min).Z(); }

		// Transforms this bounding box using given transformation matrix,
		// not to be re-used! Performing transformation many times will result loose of precision
		spdAABB Transform(const Mat44V& mtx) const
		{
			Vec3V verts[8];
			GetVertices(verts);

			spdAABB result = Empty();
			for (Vec3V& vert : verts)
				result = result.AddPoint(vert.Transform(mtx));

			return result;
		}

		spdAABB AddPoint(const Vec3V& point) const
		{
			return spdAABB(Min.Min(point), Max.Max(point));
		}

		spdAABB Merge(const spdAABB& other) const
		{
			return spdAABB(Min.Min(other.Min), Max.Max(other.Max));
		}

		void ComputeFrom(Vec3V* points, u32 pointCount)
		{
			Min = S_MAX;
			Max = S_MIN;
			for (u32 i = 0; i < pointCount; i++)
			{
				Vec3V& point = points[i];
				Min = Min.Min(point);
				Max = Max.Max(point);
			}
		}

		void ComputeFrom(Vector3* points, u32 pointCount)
		{
			Min = S_MAX;
			Max = S_MIN;
			for (u32 i = 0; i < pointCount; i++)
			{
				Vector3& point = points[i];
				Min = Min.Min(point);
				Max = Max.Max(point);
			}
		}

		// Computes one BB that will contain all given BB's
		void ComputeFrom(spdAABB* bbs, u32 bbCount)
		{
			Min = S_MAX;
			Max = S_MIN;
			for (u32 i = 0; i < bbCount; i++)
			{
				spdAABB& bb = bbs[i];
				Min = Min.Min(bb.Min);
				Min = Min.Min(bb.Max);
				Max = Max.Max(bb.Min);
				Max = Max.Max(bb.Max);
			}
		}

		spdSphere ToBoundingSphere() const
		{
			Vec3V center = (Min + Max) * S_HALF;
			Vec3V diagonal = Max - Min;

			return spdSphere(center, diagonal.Length() * S_HALF);
		}

		//  Bottom Face          Top Face		(Faces are in XY plane, Z is depth)
		// 
		//  BTL (Min)   BTR      TTL         TTR
		//  ┌─────────────┐      ┌─────────────┐
		//  │             │      │             │
		//  │             │      │             │
		//  │             │      │             │
		//  │             │      │             │
		//  │             │      │             │
		//  │             │      │             │
		//  └─────────────┘      └─────────────┘
		//  BBL         BBR      TBL         TBR (Max)
		// 
		// Naming: e.g. TBL
		// Reads as: Top Face, Bottom Left

		Vec3V BTL() const { return Min; }
		Vec3V BTR() const { return Vec3V(Max.X(), Min.Y(), Min.Z()); }
		Vec3V BBL() const { return Vec3V(Min.X(), Max.Y(), Min.Z()); }
		Vec3V BBR() const { return Vec3V(Max.X(), Max.Y(), Min.Z()); }
		Vec3V TTL() const { return Vec3V(Min.X(), Min.Y(), Max.Z()); }
		Vec3V TTR() const { return Vec3V(Max.X(), Min.Y(), Max.Z()); }
		Vec3V TBL() const { return Vec3V(Min.X(), Max.Y(), Max.Z()); }
		Vec3V TBR() const { return Max; }

		// For Vec3V[8]
		void GetVertices(Vec3V* outVertices) const
		{
			outVertices[0] = BTL();
			outVertices[1] = BBL();
			outVertices[2] = BBR();
			outVertices[3] = BTR();
			outVertices[4] = TTL();
			outVertices[5] = TBL();
			outVertices[6] = TBR();
			outVertices[7] = TTR();
		}

		bool operator==(const spdAABB& other) const
		{
			return Min.AlmostEqual(other.Min) && Max.AlmostEqual(other.Max);
		}
	};
}
