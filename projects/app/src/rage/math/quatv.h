//
// File: quatv.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "vecv.h"

namespace rage
{
	static const VecV QUAT_IDENTITY_VEC = { 0.0f, 0.0f, 0.0f, 1.0f };

	struct QuatV : VecV
	{
		QuatV() { M = QUAT_IDENTITY_VEC.M; }
		QuatV(float x, float y, float z, float w) { M = DirectX::XMVectorSet(x, y, z, w); }
		QuatV(__m128 m) { M = m; }

		bool IsIdentity() const;

		Vec3V ToEuler() const;

		static QuatV RotationNormal(const Vec3V& normal, float angle);
		static QuatV FromEuler(const Vec3V& euler);
		static QuatV FromTransform(const Mat44V& matrix);
	};

	static const QuatV QUAT_IDENTITY = { 0.0f, 0.0f, 0.0f, 1.0f };
}
