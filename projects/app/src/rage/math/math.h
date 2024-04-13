//
// File: math.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <cmath>

namespace rage
{
	static constexpr float PI = 3.14159265358979323846f;
	static constexpr float PI2 = PI * 2.0f;
	static constexpr float FOUR_THIRDS = 4.0f / 3.0f;
	static constexpr float ONE_THIRD = 1.0f / 3.0f;

	static bool AlmostEquals(float a, float b, float maxDelta = 0.01f) { return abs(a - b) <= maxDelta; }

	template<typename T>
	static const T& Max(const T& left, const T& right) { return left >= right ? left : right; }

	template<typename T>
	static const T& Min(const T& left, const T& right) { return left <= right ? left : right; }

	template<typename T>
	static constexpr float Clamp(T value, T min, T max)
	{
		if (value < min) return min;
		if (value > max) return max;
		return value;
	}

	// Remaps given value from one range to another
	static constexpr float Remap(float value, float fromMin, float fromMax, float toMin, float toMax)
	{
		float a = value - fromMin;
		float b = a / (fromMax - fromMin);
		float c = b * (toMax - toMin);
		return toMin + c;
	}

	static constexpr float DegToRad(float deg)
	{
		return deg / 180.0f * PI;
	}

	static constexpr float RadToDeg(float rad)
	{
		return rad / PI * 180.0f;
	}
}
