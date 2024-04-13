//
// File: math.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/math/math.h"

namespace rage
{
	inline float CalculateSphereAngularInertia(float mass, float radius)
	{
		return 0.4f * mass * radius * radius;
	}

	inline float CalculateSphereVolume(float radius)
	{
		return 4.0f / 3.0f * PI * radius * radius * radius;
	}

	inline Vec3V CalculateCylinderAngularInertia(float mass, float radius, float length)
	{
		float radialMom = mass * radius * radius * 0.5f;
		float centralMom = radialMom * 0.5f + mass * length * length / 12.0f;
		return Vec3V(centralMom, radialMom, centralMom);
	}

	inline float CalculateCylinderVolume(float radius, float length)
	{
		return PI * radius * radius * length;
	}


	inline Vec3V CalculateCapsuleAngularInertia(float mass, float radius, float length)
	{
		float inverse = 1.0f / (length + FOUR_THIRDS * radius);
		float radius2 = radius * radius;
		float radialMom = 0.5f * mass * radius2 * (length + 16.0f / 15.0f * radius) * inverse;
		float length2 = length * length;
		float centralMom = ONE_THIRD * mass * 
			(0.25f * length * length2 + radius * length2 + 2.25f * length * radius2 + 1.6f * radius * radius2) * inverse;
		return Vec3V(centralMom, radialMom, centralMom);
	}

	inline float CalculateCapsuleVolume(float radius, float length)
	{
		return PI * radius * radius * (length + FOUR_THIRDS * radius);
	}
}
