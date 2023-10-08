//
// File: mtxv.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "quatv.h"
#include "vecv.h"

namespace rage
{
	static const Vec4V M_IDENTITY_R0 = { 1.0f, 0.0f, 0.0f, 0.0f };
	static const Vec4V M_IDENTITY_R1 = { 0.0f, 1.0f, 0.0f, 0.0f };
	static const Vec4V M_IDENTITY_R2 = { 0.0f, 0.0f, 1.0f, 0.0f };
	static const Vec4V M_IDENTITY_R3 = { 0.0f, 0.0f, 0.0f, 1.0f };

	struct alignas(16) Mat44V
	{
		union
		{
			DirectX::XMMATRIX M;
			struct
			{
				Vec4V Right;
				Vec4V Front;
				Vec4V Up;
				Vec4V Pos;
			};
			struct
			{
				Vec4V R[4];
			};
		};

		Mat44V()
		{
			R[0] = M_IDENTITY_R0;
			R[1] = M_IDENTITY_R1;
			R[2] = M_IDENTITY_R2;
			R[3] = M_IDENTITY_R3;
		}
		Mat44V(const DirectX::XMMATRIX& m) { M = m; }

		Mat44V Multiply(const Mat44V& other) const { return XMMatrixMultiply(M, other.M); }
		Mat44V Inverse() const { return XMMatrixInverse(NULL, M); }
		bool Decompose(Vec3V* translation, Vec3V* scale, QuatV* rotation) const
		{
			Vec3V t, s;
			QuatV r;
			bool result = XMMatrixDecompose(&s.M, &r.M, &t.M, M);
			if (translation) *translation = t;
			if (scale) *scale = s;
			if (rotation) *rotation = r;
			return result;
		}

		Mat44V operator!() const { return Inverse(); }
		Mat44V operator*(const Mat44V& other) const { return Multiply(other); }
		Mat44V& operator*=(const Mat44V& other) { M = XMMatrixMultiply(M, other.M); return *this; }

		static Mat44V FromNormalPos(const Vec3V& pos, const Vec3V& normal)
		{
			Vec3V right = normal.Cross(VEC_UP);
			if (normal.Dot(VEC_UP) > 0.9995f) // Fix orientation when normal is aligned with up
				right = VEC_RIGHT;
			Vec3V up = right.Cross(normal).Normalized();
			Mat44V m;
			m.Front = Vec4V(normal, 0.0f);
			m.Right = Vec4V(right, 0.0f);
			m.Up = Vec4V(up, 0.0f);
			m.Pos = Vec4V(pos, 1.0f);
			return m;
		}
		static Mat44V Transform(const Vec3V& scale, const QuatV& rotation, const Vec3V& translation)
		{
			return DirectX::XMMatrixTransformation(
				S_ZERO, QUAT_IDENTITY, scale,
				S_ZERO, rotation,
				translation);
		}
		static Mat44V Identity() { return Mat44V(); }
		static Mat44V Translation(const Vec3V& translation)
		{
			Mat44V m;
			m.Pos = Vec4V(translation);
			return m;
		}
		static Mat44V Scale(const Vec3V& scale)
		{
			Mat44V m;
			m.R[0].SetX(scale.X());
			m.R[1].SetX(scale.Y());
			m.R[2].SetX(scale.Z());
			return m;
		}
		static Mat44V Rotation(const QuatV& rotation) { return DirectX::XMMatrixRotationQuaternion(rotation.M); }
		static Mat44V RotationNormal(const Vec3V& normal, float angle) { return DirectX::XMMatrixRotationNormal(normal, angle); }
	};

	struct alignas(16) Mat34V
	{
		union
		{
			struct
			{
				Vec3V Right;
				Vec3V Front;
				Vec3V Up;
				Vec3V Pos;
			};
			struct
			{
				Vec3V R[4];
			};
		};

		Mat34V()
		{
			R[0] = M_IDENTITY_R0;
			R[1] = M_IDENTITY_R1;
			R[2] = M_IDENTITY_R2;
			R[3] = M_IDENTITY_R3;
		}

		Mat34V(const Mat44V& mat)
		{
			R[0] = mat.R[0];
			R[1] = mat.R[1];
			R[2] = mat.R[2];
			R[3] = mat.R[3];
		}

		Mat44V To44() const
		{
			Mat44V m44;
			m44.R[0] = Vec4V(R[0], 0.0f);
			m44.R[1] = Vec4V(R[1], 0.0f);
			m44.R[2] = Vec4V(R[2], 0.0f);
			m44.R[3] = Vec4V(R[3], 1.0f);
			return m44;
		}

		static Mat34V Identity()
		{
			Mat34V m;
			m.R[0] = M_IDENTITY_R0;
			m.R[1] = M_IDENTITY_R1;
			m.R[2] = M_IDENTITY_R2;
			return m;
		}
	};
}
