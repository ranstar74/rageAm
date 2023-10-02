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

rage::Vec3V::Vec3V(const Vec4V& s)
{
	M = s.M;
}

rage::Vec3V::Vec3V(const Vector3& v)
{
	M = _mm_loadu_ps(&v.X);
}

rage::Vec3V rage::Vec3V::Project(const Vec3V& normal) const
{
	ScalarV dot = Dot(normal);
	return DirectX::XMVectorMultiply(normal.M, dot.M);
}

rage::Vec3V rage::Vec3V::Min(const Vec3V& other) const
{
	// Min function takes into account W component
	DirectX::XMVectorSetW(M, INFINITY);
	DirectX::XMVectorSetW(other.M, INFINITY);
	return DirectX::XMVectorMin(M, other.M);
}

rage::Vec3V rage::Vec3V::Max(const Vec3V& other) const
{
	// Same as in Min
	DirectX::XMVectorSetW(M, -INFINITY);
	DirectX::XMVectorSetW(other.M, -INFINITY);
	return DirectX::XMVectorMax(M, other.M);
}

rage::Vec3V rage::Vec3V::Transform(const Mat44V& mtx) const
{
	Vec4V temp = XMVector3Transform(M, mtx);
	temp /= temp.W();
	return temp;
}

rage::Vec4V rage::Vec3V::Transform4(const Mat44V& mtx) const
{
	return XMVector3Transform(M, mtx);
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
