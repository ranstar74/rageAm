//
// File: sphere.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/math/math.h"
#include "rage/math/vecv.h"

namespace rage
{
	/**
	 * \brief Sphere with center and radius.
	 */
	struct spdSphere
	{
		Vec4V CenterAndRadius;

		spdSphere() = default;
		spdSphere(const Vec3V& center, const ScalarV& radius)
		{
			SetCenter(center);
			SetRadius(radius);
		}

		void    SetCenter(const Vec3V& v) { CenterAndRadius.SetXYZ(v); }
		Vec3V   GetCenter() const { return Vec3V(CenterAndRadius); }
		void    SetRadius(const ScalarV& s) { CenterAndRadius.SetW(s.Get()); }
		ScalarV GetRadius() const { return CenterAndRadius.WWWW(); }

		ScalarV DistanceTo(const spdSphere& other) const { return GetCenter().DistanceTo(other.GetCenter()); }

		spdSphere Merge(const spdSphere& other) const
		{
			Vec3V to = other.GetCenter() - GetCenter();
			float dist = to.Length().Get();
			float lRadius = CenterAndRadius.W();
			float rRadius = other.CenterAndRadius.W();
			float min = Min(-lRadius, dist - rRadius);
			float max = Max(lRadius, dist + rRadius);
			float centre = (min + max) / 2.0f;
			float newRadius = (max - min) / 2.0f;
			Vec3V offsetToNewCenter = to * centre / Max(0.001f, dist);
			return spdSphere(GetCenter() + offsetToNewCenter, newRadius);
		}

		bool operator==(const spdSphere& other) const
		{
			return CenterAndRadius.AlmostEqual(other.CenterAndRadius);
		}
	};
}
