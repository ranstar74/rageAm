//
// File: sphere.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

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

		void SetCenter(const Vec3V& v) { CenterAndRadius.SetXYZ(v); }
		Vec3V GetCenter() const { return Vec3V(CenterAndRadius); }

		void SetRadius(const ScalarV& s) { CenterAndRadius.SetW(s.Get()); }
		ScalarV GetRadius() const { return CenterAndRadius.WWWW(); }
	};
}
