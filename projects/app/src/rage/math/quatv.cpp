#include "quatv.h"
#include "vec.h"
#include "mtxv.h"

rage::Vec3V rage::QuatV::ToEuler() const
{
	// https://github.com/RikoOphorst/dxr-path-tracing/blob/c94956c12a2269008e9919161d4f96f277e27c87/src/rtrt/model.cc#L9
	Vector3 output;
	float sqw;
	float sqx;
	float sqy;
	float sqz;

	DirectX::XMFLOAT4 quaternion;
	DirectX::XMStoreFloat4(&quaternion, M);

	sqw = quaternion.w * quaternion.w;
	sqx = quaternion.x * quaternion.x;
	sqy = quaternion.y * quaternion.y;
	sqz = quaternion.z * quaternion.z;

	float unit = sqw + sqx + sqy + sqz;
	float test = quaternion.x * quaternion.w - quaternion.y * quaternion.z;

	if (test > 0.4995f * unit)
	{
		output.Y = (2.0f * atan2(quaternion.y, quaternion.x));
		output.X = (DirectX::XM_PIDIV2);

		return output;
	}
	if (test < -0.4995f * unit)
	{
		output.Y = (-2.0f * atan2(quaternion.y, quaternion.x));
		output.X = (-DirectX::XM_PIDIV2);

		return output;
	}

	output.X = (asin(2.0f * (quaternion.w * quaternion.x - quaternion.z * quaternion.y)));
	output.Y = (atan2(2.0f * quaternion.w * quaternion.y + 2.0f * quaternion.z * quaternion.x, 1 - 2.0f * (sqx + sqy)));
	output.Z = (atan2(2.0f * quaternion.w * quaternion.z + 2.0f * quaternion.x * quaternion.y, 1 - 2.0f * (sqz + sqx)));

	return output;
}

bool rage::QuatV::IsIdentity() const
{
	return AlmostEqual(QUAT_IDENTITY);
}

rage::QuatV rage::QuatV::RotationNormal(const Vec3V& normal, float angle)
{
	return DirectX::XMQuaternionRotationNormal(normal, angle);
}

rage::QuatV rage::QuatV::FromEuler(const Vec3V& euler)
{
	return DirectX::XMQuaternionRotationRollPitchYawFromVector(euler.ZXY());
}

rage::QuatV rage::QuatV::FromTransform(const Mat44V& matrix)
{
	return XMQuaternionRotationMatrix(matrix.M);
}
