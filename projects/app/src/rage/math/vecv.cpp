#include "vecv.h"

#include "vec.h"
#include "mtxv.h"
#include "rage/paging/resource.h"

rage::VecV::VecV()
{
	if (datResource::IsBuilding())
		return;

	M = DirectX::XMVectorReplicate(0);
}

rage::VecV rage::VecV::Negate() const
{
	return DirectX::XMVectorSubtract(S_ZERO.M, M);
}

bool rage::VecV::operator==(const VecV& other) const
{
	__m128 cmp = DirectX::XMVectorEqual(M, other.M);
	return (_mm_movemask_ps(cmp) & 0xF) == 0xF;
}

bool rage::VecV::operator>=(const VecV& other) const
{
	__m128 cmp = DirectX::XMVectorGreaterOrEqual(M, other.M);
	return (_mm_movemask_ps(cmp) & 0xF) == 0xF;
}

bool rage::VecV::operator<=(const VecV& other) const
{
	__m128 cmp = DirectX::XMVectorLessOrEqual(M, other.M);
	return (_mm_movemask_ps(cmp) & 0xF) == 0xF;
}

bool rage::VecV::operator>(const VecV& other) const
{
	__m128 cmp = DirectX::XMVectorGreater(M, other.M);
	return (_mm_movemask_ps(cmp) & 0xF) == 0xF;
}

bool rage::VecV::operator<(const VecV& other) const
{
	__m128 cmp = DirectX::XMVectorLess(M, other.M);
	return (_mm_movemask_ps(cmp) & 0xF) == 0xF;
}

bool rage::VecV::AlmostEqual(const VecV& other) const
{
	__m128 diff = DirectX::XMVectorSubtract(M, other.M);
	__m128 abs = DirectX::XMVectorAbs(diff);
	__m128 cmp = DirectX::XMVectorLessOrEqual(abs, S_EPSION.M);
	return (_mm_movemask_ps(cmp) & 0xF) == 0xF;
}

float rage::VecV::operator[](int index) const
{
	if (index == 0) return X();
	if (index == 1) return Y();
	if (index == 2) return Z();
	if (index == 3) return W();

	AM_UNREACHABLE("VecV::operator[] -> Index %i is out of bounds.", index);
}

rage::ScalarV rage::ScalarV::Min(const ScalarV& other) const
{
	return DirectX::XMVectorMin(M, other.M);
}

rage::ScalarV rage::ScalarV::Max(const ScalarV& other) const
{
	return DirectX::XMVectorMax(M, other.M);
}

rage::ScalarV rage::ScalarV::Clamp(const ScalarV& min, const ScalarV& max) const
{
	return DirectX::XMVectorClamp(M, min.M, max.M);
}

rage::ScalarV rage::ScalarV::Lerp(const ScalarV& to, const ScalarV& t) const
{
	return DirectX::XMVectorLerpV(M, to.M, t.M);
}

rage::Vec3V::Vec3V(const Vec4V& v)
{
	M = v.M;
}

rage::Vec3V::Vec3V(const Vector3& v)
{
	// Fix for loadu accessing read-protected memory,
	// attempting to read all 16 bytes, but Vector3 is only 3 floats (12 bytes)
	Vector3 v_ = v;
	M = _mm_loadu_ps(&v_.X);
}

rage::Vec3V rage::Vec3V::Project(const Vec3V& normal) const
{
	ScalarV dot = Dot(normal);
	return DirectX::XMVectorMultiply(normal.M, dot.M);
}

void rage::Vec3V::TangentAndBiNormal(Vec3V& outTangent, Vec3V& outBiNormal) const
{
	// Pick vector that's not aligned with 'this'
	if (Dot(VEC_RIGHT).Abs().AlmostEqual(S_ONE))
		outTangent = Cross(VEC_FRONT);
	else 
		outTangent = Cross(VEC_RIGHT);
	outTangent.Normalize();
	outBiNormal = Cross(outTangent);
}

rage::Vec3V rage::Vec3V::Tangent() const
{
	// Pick vector that's not aligned with 'this'
	if (Dot(VEC_RIGHT).Abs().AlmostEqual(S_ONE))
		return Cross(VEC_FRONT).Normalized();
	return Cross(VEC_RIGHT).Normalized();
}

rage::Vec3V rage::Vec3V::BiNormal() const
{
	return Tangent().Cross(M);
}

bool rage::Vec3V::IsParallel(const Vec3V& to) const
{
	return Dot(to).Abs().AlmostEqual(S_ONE);
}

rage::Vec3V rage::Vec3V::Min(const Vec3V& other) const
{
	return _mm_min_ps(M, other.M);
}

rage::Vec3V rage::Vec3V::Max(const Vec3V& other) const
{
	return _mm_max_ps(M, other.M);
}

rage::Vec3V rage::Vec3V::Transform(const Mat44V& mtx) const
{
	Vec4V temp = XMVector3Transform(M, mtx.M);
	temp /= temp.W();
	return temp;
}

rage::Vec3V rage::Vec3V::TransformNormal(const Mat44V& mtx) const
{
	Mat44V copy = mtx;
	copy.Pos = Vec4V(0, 0, 0, 1);
	return Transform(copy).Normalized();
}

rage::Vec4V rage::Vec3V::Transform4(const Mat44V& mtx) const
{
	return XMVector3Transform(M, mtx.M);
}

rage::Vec3V rage::Vec3V::Rotate(const Vec3V& axis, float angle) const
{
	return DirectX::XMVector3Rotate(M, DirectX::XMQuaternionRotationNormal(axis, angle));
}

rage::Vec3V rage::Vec3V::Rotate(const QuatV& rotation) const
{
	return DirectX::XMVector3Rotate(M, rotation.M);
}

rage::Vec3V rage::Vec3V::ToDegrees() const
{
	return operator*(S_RAD2DEG);
}

rage::Vec3V rage::Vec3V::ToRadians() const
{
	return operator*(S_DEG2RAD);
}

bool rage::Vec3V::AlmostEqual(const Vec3V& other) const
{
	__m128 diff = DirectX::XMVectorSubtract(M, other.M);
	__m128 abs = DirectX::XMVectorAbs(diff);
	return DirectX::XMVector3LessOrEqual(abs, S_EPSION.M);
}

bool rage::Vec3V::AlmostEqual(const ScalarV& other) const
{
	__m128 diff = DirectX::XMVectorSubtract(M, other.M);
	__m128 abs = DirectX::XMVectorAbs(diff);
	return DirectX::XMVector3LessOrEqual(abs, S_EPSION.M);
}

bool rage::Vec3V::AlmostEqual(const Vec3V& other, const ScalarV& epsilon) const
{
	__m128 diff = DirectX::XMVectorSubtract(M, other.M);
	__m128 abs = DirectX::XMVectorAbs(diff);
	return DirectX::XMVector3LessOrEqual(abs, epsilon.M);
}

bool rage::Vec3V::AlmostEqual(const ScalarV& other, const ScalarV& epsilon) const
{
	__m128 diff = DirectX::XMVectorSubtract(M, other.M);
	__m128 abs = DirectX::XMVectorAbs(diff);
	return DirectX::XMVector3LessOrEqual(abs, epsilon.M);
}

float rage::Vec3V::operator[](int index) const
{
	if (index == 0) return X();
	if (index == 1) return Y();
	if (index == 2) return Z();

	AM_UNREACHABLE("Vec3V::operator[] -> Index %i is out of bounds.", index);
}

void rage::Vec4V::SetXYZ(const Vec3V& v)
{
	// v.Z, v.W, this.X, this.W
	__m128 temp = _mm_shuffle_ps(v.M, M, _MM_SHUFFLE(3, 0, 3, 2));
	// v.X, v.Y, V.X, this.W
	M = _mm_shuffle_ps(v.M, temp, _MM_SHUFFLE(2, 0, 1, 0));
}
