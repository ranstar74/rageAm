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

	struct Mat34V
	{
		Vec3V R[3];

		static Mat34V Identity()
		{
			Mat34V m;
			m.R[0] = M_IDENTITY_R0;
			m.R[1] = M_IDENTITY_R1;
			m.R[2] = M_IDENTITY_R2;
			return m;
		}
	};

	struct Mat44V : DirectX::XMMATRIX
	{
		//Vec4V R[4];

		static Mat44V Identity()
		{
			//Mat44V m;
			//m.R[0] = M_IDENTITY_R0;
			//m.R[1] = M_IDENTITY_R1;
			//m.R[2] = M_IDENTITY_R2;
			//m.R[3] = M_IDENTITY_R3;
			//return m;
			return Mat44V(DirectX::XMMatrixIdentity());
		}

		static Mat44V Translation(float x, float y, float z)
		{
			// TODO: ...
			//auto mtx = DirectX::XMMatrixTranslation(x, y, z);
			//Mat44V m;
			//m.R[0] = mtx.r[0];
			//m.R[1] = mtx.r[1];
			//m.R[2] = mtx.r[2];
			//m.R[3] = mtx.r[3];
			//return m;
			return Mat44V(DirectX::XMMatrixTranslation(x, y, z));
		}

		// Transformations are applied in the order they appear in parameters
		static Mat44V Transform(const Vec3V& scale, const QuatV& rotation, const Vec3V& translation)
		{
			return Mat44V(DirectX::XMMatrixTransformation(
				S_ZERO, QUAT_IDENTITY, scale,
				S_ZERO, rotation,
				translation));
		}

		bool Decompose(Vec3V* outTrans, Vec3V* outScale, QuatV* outRot) const
		{
			Vec3V trans, scale;
			QuatV rot;

			if (!XMMatrixDecompose(&scale.M, &rot.M, &trans.M, *this))
				return false;

			if (outTrans) *outTrans = trans;
			if (outScale) *outScale = scale;
			if (outRot) *outRot = rot;

			return true;
		}

		Mat44V Inverse() const
		{
			/*auto mtx = DirectX::XMMatrixInverse(NULL, );
			Mat44V m;
			m.R[0] = mtx.r[0];
			m.R[1] = mtx.r[1];
			m.R[2] = mtx.r[2];
			m.R[3] = mtx.r[3];
			return m;*/
			return Mat44V(XMMatrixInverse(NULL, *this));
		}

		Mat44V& operator*=(const Mat44V& other)
		{
			*this = Mat44V(XMMatrixMultiply(*this, other));
			return *this;
		}

		Mat44V operator*(const Mat44V& other) const
		{
			return Mat44V(XMMatrixMultiply(*this, other));
		}
	};
}
