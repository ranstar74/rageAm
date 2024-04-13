//
// File: quatv.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
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

		Vec3V ToEuler() const
		{
			Vec3V euler;
			Vec3V axis;
			float angle;
			axis = VEC_RIGHT;
			DirectX::XMQuaternionToAxisAngle(&axis.M, &angle, M);
			euler.SetX(angle);
			axis = VEC_FRONT;
			DirectX::XMQuaternionToAxisAngle(&axis.M, &angle, M);
			euler.SetY(angle);
			axis = VEC_UP;
			DirectX::XMQuaternionToAxisAngle(&axis.M, &angle, M);
			euler.SetZ(angle);
			return euler;
		}
	};

	static const QuatV QUAT_IDENTITY = { 0.0f, 0.0f, 0.0f, 1.0f };
}
