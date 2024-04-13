//
// File: vecv.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <xmmintrin.h>
#include <DirectXMath.h>

namespace rage
{
	struct QuatV;
	struct Mat44V;
	struct Vector3;
	struct Vec3V;
	struct Vec4V;
	struct ScalarV;

	/**
	 * \brief SIMD-accelerated vector (__m128)
	 */
	struct alignas(16) VecV
	{
		__m128 M;

		VecV();
		VecV(float x, float y, float z, float w) { M = DirectX::XMVectorSet(x, y, z, w); }
		VecV(float v) { M = DirectX::XMVectorReplicate(v); }
		VecV(__m128 m) { M = m; }

		float X() const { return DirectX::XMVectorGetX(M); }
		float Y() const { return DirectX::XMVectorGetY(M); }
		float Z() const { return DirectX::XMVectorGetZ(M); }
		float W() const { return DirectX::XMVectorGetW(M); }

		void SetX(float v) { M = DirectX::XMVectorSetX(M, v); }
		void SetY(float v) { M = DirectX::XMVectorSetY(M, v); }
		void SetZ(float v) { M = DirectX::XMVectorSetZ(M, v); }
		void SetW(float v) { M = DirectX::XMVectorSetW(M, v); }

		VecV Negate() const;

		bool operator==(const VecV& other) const;
		bool operator>=(const VecV& other) const;
		bool operator<=(const VecV& other) const;
		bool operator>(const VecV& other) const;
		bool operator<(const VecV& other) const;

		bool AlmostEqual(const VecV& other) const;

		float operator[](int index) const;

		operator __m128() const { return M; }
	};

	/**
	 * \brief Scalar represents SIMD vector with all components set to X.
	 */
	struct ScalarV : VecV
	{
		ScalarV(float v) : VecV(v) {}
		ScalarV(__m128 m) : VecV(m) {}
		ScalarV() = default;

		float Get() const { return X(); }
		void Set(float v) { M = DirectX::XMVectorReplicate(v); }

		// -- Common Operations --

		ScalarV Reciprocal() const { return DirectX::XMVectorReciprocal(M); }
		ScalarV ReciprocalEstimate() const { return DirectX::XMVectorReciprocalEst(M); }
		ScalarV ReciprocalSqrt() const { return DirectX::XMVectorReciprocalSqrt(M); }
		ScalarV ReciprocalSqrtEstimate() const { return DirectX::XMVectorReciprocalSqrtEst(M); }
		ScalarV Sqrt() const { return DirectX::XMVectorSqrt(M); }
		ScalarV Abs() const { return DirectX::XMVectorAbs(M); }

		ScalarV Min(const ScalarV& other) const;
		ScalarV Max(const ScalarV& other) const;

		ScalarV Clamp(const ScalarV& min, const ScalarV& max) const;
		ScalarV Lerp(const ScalarV& to, const ScalarV& t) const;

		// -- Arithmetic Operators --

		ScalarV operator+(const VecV& other) const { return DirectX::XMVectorAdd(M, other.M); }
		ScalarV operator-(const VecV& other) const { return DirectX::XMVectorSubtract(M, other.M); }
		ScalarV& operator+=(const VecV& other) { M = DirectX::XMVectorAdd(M, other.M); return *this; }
		ScalarV& operator-=(const VecV& other) { M = DirectX::XMVectorSubtract(M, other.M); return *this; }
		ScalarV operator*(const VecV& other) const { return DirectX::XMVectorMultiply(M, other.M); }
		ScalarV operator/(const VecV& other) const { return DirectX::XMVectorDivide(M, other.M); }
		ScalarV& operator*=(const VecV& other) { M = DirectX::XMVectorMultiply(M, other.M); return *this; }
		ScalarV& operator/=(const VecV& other) { M = DirectX::XMVectorDivide(M, other.M); return *this; }

		ScalarV operator-() const { return Negate().M; }

		// -- Getters --

		float operator[](int index) const = delete;
	};

	/**
	 * \brief SIMD-accelerated vector with 3 components XYZ.
	 */
	struct Vec3V : VecV
	{
		Vec3V(float x, float y, float z) : VecV(x, y, z, x) {}
		Vec3V(float x, float y) : Vec3V(x, y, 1.0f) {}
		Vec3V(float v) : VecV(v) {}
		Vec3V(const Vec4V& v);
		Vec3V(const ScalarV& s) { M = s.M; }
		Vec3V(const Vector3& v);
		Vec3V(__m128 m) : VecV(m) {}
		Vec3V() = default;

		float W() const = delete;
		void SetW(float v) = delete;

		// -- Common Operations --

		ScalarV DistanceTo(const Vec3V& other) const { return (*this - other).Length(); }
		ScalarV DistanceToEstimate(const Vec3V& other) const { return (*this - other).LengthEstimate(); }
		ScalarV DistanceToSquared(const Vec3V& other) const { return (*this - other).LengthSquared(); }

		ScalarV Length() const { return DirectX::XMVector3Length(M); }
		ScalarV LengthSquared() const { return DirectX::XMVector3LengthSq(M); }
		ScalarV LengthEstimate() const { return DirectX::XMVector3LengthEst(M); }

		void Normalize() { M = DirectX::XMVector3Normalize(M); }
		void NormalizeEstimate() { M = DirectX::XMVector3NormalizeEst(M); }
		Vec3V Normalized() const { return DirectX::XMVector3Normalize(M); }
		Vec3V NormalizedEstimate() const { return DirectX::XMVector3NormalizeEst(M); }

		ScalarV Dot(const Vec3V& v) const { return ScalarV(DirectX::XMVector3Dot(M, v.M)); }
		Vec3V Cross(const Vec3V& v) const { return Vec3V(DirectX::XMVector3Cross(M, v.M)); }
		Vec3V Reflect(const Vec3V& normal) const { return DirectX::XMVector3Reflect(M, normal.M); }
		Vec3V Project(const Vec3V& normal) const;
		void TangentAndBiNormal(Vec3V& outTangent, Vec3V& outBiNormal) const;
		bool IsParallel(const Vec3V& to) const;

		Vec3V Reciprocal() const { return DirectX::XMVectorReciprocal(M); }
		Vec3V ReciprocalEstimate() const { return DirectX::XMVectorReciprocalEst(M); }
		Vec3V ReciprocalSqrt() const { return DirectX::XMVectorReciprocalSqrt(M); }
		Vec3V ReciprocalSqrtEstimate() const { return DirectX::XMVectorReciprocalSqrtEst(M); }

		Vec3V Lerp(const Vec3V& to, const ScalarV& t) const { return DirectX::XMVectorLerpV(M, to.M, t.M); }

		Vec3V Min(const Vec3V& other) const;
		Vec3V Max(const Vec3V& other) const;

		Vec3V Transform(const Mat44V& mtx) const;
		Vec4V Transform4(const Mat44V& mtx) const;

		Vec3V Rotate(const Vec3V& axis, float angle) const;
		Vec3V Rotate(const QuatV& rotation) const;

		Vec3V Abs() const { return DirectX::XMVectorAbs(M); }

		Vec3V& operator=(const Vec3V& other) { M = other.M; return *this; }

		// -- Arithmetic Operators --

		Vec3V operator+(const VecV& other) const { return DirectX::XMVectorAdd(M, other.M); }
		Vec3V operator-(const VecV& other) const { return DirectX::XMVectorSubtract(M, other.M); }
		Vec3V& operator+=(const VecV& other) { M = DirectX::XMVectorAdd(M, other.M); return *this; }
		Vec3V& operator-=(const VecV& other) { M = DirectX::XMVectorSubtract(M, other.M); return *this; }
		Vec3V operator*(const VecV& other) const { return DirectX::XMVectorMultiply(M, other.M); }
		Vec3V operator/(const VecV& other) const { return DirectX::XMVectorDivide(M, other.M); }
		Vec3V& operator*=(const VecV& other) { M = DirectX::XMVectorMultiply(M, other.M); return *this; }
		Vec3V& operator/=(const VecV& other) { M = DirectX::XMVectorDivide(M, other.M); return *this; }

		Vec3V operator-() const { return Negate().M; }

		// -- Comparison Operators --

		// Disable Vec3V comparison against Vec4V

		bool operator==(const VecV& other) const = delete;
		bool operator>=(const VecV& other) const = delete;
		bool operator<=(const VecV& other) const = delete;
		bool operator>(const VecV& other) const = delete;
		bool operator<(const VecV& other) const = delete;
		bool AlmostEqual(const VecV& other) const = delete;

		// Vec3V vs Vec3V, Vec3V vs ScalarV

		bool operator==(const Vec3V& other) const { return DirectX::XMVector3Equal(M, other.M); }
		bool operator>=(const Vec3V& other) const { return DirectX::XMVector3GreaterOrEqual(M, other.M); }
		bool operator<=(const Vec3V& other) const { return DirectX::XMVector3LessOrEqual(M, other.M); }
		bool operator>(const Vec3V& other) const { return DirectX::XMVector3Greater(M, other.M); }
		bool operator<(const Vec3V& other) const { return DirectX::XMVector3Less(M, other.M); }
		bool operator==(const ScalarV& other) const { return DirectX::XMVector3Equal(M, other.M); }
		bool operator>=(const ScalarV& other) const { return DirectX::XMVector3GreaterOrEqual(M, other.M); }
		bool operator<=(const ScalarV& other) const { return DirectX::XMVector3LessOrEqual(M, other.M); }
		bool operator>(const ScalarV& other) const { return DirectX::XMVector3Greater(M, other.M); }
		bool operator<(const ScalarV& other) const { return DirectX::XMVector3Less(M, other.M); }
		bool AlmostEqual(const Vec3V& other) const;
		bool AlmostEqual(const ScalarV& other) const;
		bool AlmostEqual(const Vec3V& other, const ScalarV& epsilon) const;
		bool AlmostEqual(const ScalarV& other, const ScalarV& epsilon) const;

		// -- Getters --

		Vec3V XYZ() const { return M; }
		Vec3V YXZ() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(0, 2, 0, 1)); }
		Vec3V ZXY() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(0, 1, 0, 2)); }
		Vec3V XZY() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(0, 1, 2, 0)); }
		Vec3V YZX() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(0, 0, 2, 1)); }
		Vec3V ZYX() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(0, 0, 1, 2)); }

		ScalarV XXX() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(0, 0, 0, 0)); }
		ScalarV YYY() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(1, 1, 1, 1)); }
		ScalarV ZZZ() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(2, 2, 2, 2)); }

		float operator[](int index) const;
	};

	/**
	 * \brief SIMD-accelerated vector with 4 components XYZW.
	 */
	struct Vec4V : VecV
	{
		Vec4V(const Vec3V& v, float w = 1.0f) : VecV(v.X(), v.Y(), v.Z(), w) {}
		Vec4V(float x, float y, float z, float w) : VecV(x, y, z, w) {}
		Vec4V(const ScalarV& s) : VecV(s.M) {}
		Vec4V(__m128 m) : VecV(m) {}
		Vec4V() = default;

		ScalarV Min(const Vec4V& other) const { return DirectX::XMVectorMin(M, other.M); }
		ScalarV Max(const Vec4V& other) const { return DirectX::XMVectorMin(M, other.M); }

		void SetXYZ(const Vec3V& v);

		// -- Arithmetic Operators --

		// We don't allow operations with Vec3V because W component is undefined

		Vec4V operator+(const Vec4V& other) const { return DirectX::XMVectorAdd(M, other.M); }
		Vec4V operator-(const Vec4V& other) const { return DirectX::XMVectorSubtract(M, other.M); }
		Vec4V& operator+=(const Vec4V& other) { M = DirectX::XMVectorAdd(M, other.M); return *this; }
		Vec4V& operator-=(const Vec4V& other) { M = DirectX::XMVectorSubtract(M, other.M); return *this; }
		Vec4V operator*(const Vec4V& other) const { return DirectX::XMVectorMultiply(M, other.M); }
		Vec4V operator/(const Vec4V& other) const { return DirectX::XMVectorDivide(M, other.M); }
		Vec4V& operator*=(const Vec4V& other) { M = DirectX::XMVectorMultiply(M, other.M); return *this; }
		Vec4V& operator/=(const Vec4V& other) { M = DirectX::XMVectorDivide(M, other.M); return *this; }

		Vec4V operator+(const ScalarV& other) const { return DirectX::XMVectorAdd(M, other.M); }
		Vec4V operator-(const ScalarV& other) const { return DirectX::XMVectorSubtract(M, other.M); }
		Vec4V& operator+=(const ScalarV& other) { M = DirectX::XMVectorAdd(M, other.M); return *this; }
		Vec4V& operator-=(const ScalarV& other) { M = DirectX::XMVectorSubtract(M, other.M); return *this; }
		Vec4V operator*(const ScalarV& other) const { return DirectX::XMVectorMultiply(M, other.M); }
		Vec4V operator/(const ScalarV& other) const { return DirectX::XMVectorDivide(M, other.M); }
		Vec4V& operator*=(const ScalarV& other) { M = DirectX::XMVectorMultiply(M, other.M); return *this; }
		Vec4V& operator/=(const ScalarV& other) { M = DirectX::XMVectorDivide(M, other.M); return *this; }

		Vec4V operator-() const { return Negate().M; }

		// -- Getters --

		ScalarV XXXX() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(0, 0, 0, 0)); }
		ScalarV YYYY() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(1, 1, 1, 1)); }
		ScalarV ZZZZ() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(2, 2, 2, 2)); }
		ScalarV WWWW() const { return _mm_shuffle_ps(M, M, _MM_SHUFFLE(3, 3, 3, 3)); }
	};

	static const ScalarV S_ZERO = { 0.0f };
	static const ScalarV S_ONE = { 1.0f };
	static const ScalarV S_TWO = { 2.0f };
	static const ScalarV S_HALF = { 0.5f };
	static const ScalarV S_QUARTER = { 0.25f };
	static const ScalarV S_PI = { 3.14159265358979323846f };
	static const ScalarV S_2PI = { 2.0f * 3.14159265358979323846f };
	static const ScalarV S_FLT_MAX = { FLT_MAX };
	static const ScalarV S_MAX = { INFINITY };
	static const ScalarV S_MIN = { -INFINITY };
	static const ScalarV S_EPSION = { 0.000001f };

	static const Vec3V VEC_X = { 1.0f, 0.0f, 0.0f };
	static const Vec3V VEC_Y = { 0.0f, 1.0f, 0.0f };
	static const Vec3V VEC_Z = { 0.0f, 0.0f, 1.0f };

	static const Vec3V VEC_ZERO = { 0.0f, 0.0f, 0.0f };

	static const Vec3V VEC_ORIGIN = { 0.0f, 0.0f, 0.0f };
	static const Vec3V VEC_UP = { 0.0f, 0.0f, 1.0f };
	static const Vec3V VEC_DOWN = { 0.0f, 0.0f, -1.0f };
	static const Vec3V VEC_FORWARD = { 0.0f, 1.0f, 0.0f };
	static const Vec3V VEC_FRONT = { 0.0f, 1.0f, 0.0f };
	static const Vec3V VEC_BACK = { 0.0f, -1.0f, 0.0f };
	static const Vec3V VEC_RIGHT = { 1.0f, 0.0f, 0.0f };
	static const Vec3V VEC_LEFT = { -1.0f, 0.0f, 0.0f };

	static const Vec3V VEC_NORTH = { 0.0f, 1.0f, 0.0f };
	static const Vec3V VEC_SOUTH = { 0.0f, -1.0f, 0.0f };
	static const Vec3V VEC_EAST = { 1.0f, 0.0f, 0.0f };
	static const Vec3V VEC_WEST = { -1.0f, 0.0f, 0.0f };

	static const Vec3V VEC_EXTENT = { 1.0f, 1.0f, 1.0f };
	// Normalized (1, 1, 1)
	static const Vec3V VEC_EXTENT_SPHERE = { 0.5773502691896259f, 0.5773502691896259f, 0.5773502691896259f };
}
