//
// File: boundprimitives.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "boundbase.h"

namespace rage
{
	// Bound cylinder & capsule are pointing Y axis up
	static const Vec3V PH_DEFAULT_ORIENTATION = { 0.0f, 1.0f, 0.0f };

	/**
	 * \brief This non-standard class was added because all primitive bounds share GetMaterialID virtual function,
	 * but it's not part of phBound virtual table.
	 */
	class phBoundPrimitive : public phBound
	{
	public:
		phBoundPrimitive() = default;
		phBoundPrimitive(const datResource& rsc) : phBound(rsc) {}

		phMaterialMgr::Id GetMaterialIdFromPartIndexAndComponent(int partIndex, int boundIndex = BOUND_PARTS_ALL) const override { return GetMaterialID(); }
		phMaterial*       GetMaterial(int partIndex) const override { return phMaterialMgr::GetInstance()->GetMaterialById(GetPrimitiveMaterialId()); }
		void              SetMaterial(phMaterialMgr::Id materialId, int partIndex = BOUND_PARTS_ALL) override;
		virtual u64       GetMaterialID() const;
	};

	/**
	 * \brief Axis-Aligned Box Collider.
	 */
	class phBoundBox : public phBoundPrimitive
	{
	public:
		phBoundBox() : phBoundBox(spdAABB(-S_ONE, S_ONE)) {}
		phBoundBox(const spdAABB& aabb);
		phBoundBox(const datResource& rsc) : phBoundPrimitive(rsc) {}

		void CalculateExtents() override;
	};

	class phBoundSphere : public phBoundPrimitive
	{
	public:
		phBoundSphere() : phBoundSphere(VEC_ORIGIN, 1.0f) {}
		phBoundSphere(const Vec3V& center, float radius);
		phBoundSphere(const datResource& rsc) : phBoundPrimitive(rsc) {}

		void CalculateExtents() override;

		float GetRadius() const { return m_RadiusAroundCentroid; }
		void  SetRadius(float r);
	};

	class phBoundCylinder : public phBoundPrimitive
	{
		u8 m_Pad[16] = {};

		void CalculateExtents(float radius, float halfHeight);
	public:
		phBoundCylinder() : phBoundCylinder(VEC_ORIGIN, 1.0f, 1.0f) {}
		phBoundCylinder(const Vec3V& center, float radius, float halfHeight);
		phBoundCylinder(const datResource& rsc) : phBoundPrimitive(rsc) {}

		void CalculateExtents() override;

		Vec3V GetHalfExtents() const { return m_BoundingBoxMax - m_CentroidOffset; }
		float GetRadius() const { return GetHalfExtents().X(); }
		float GetHalfHeight() const { return GetHalfExtents().Y(); }
		float GetHeight() const { return m_BoundingBoxMax.Y - m_BoundingBoxMin.Y; }
	};

	class phBoundCapsule : public phBoundPrimitive
	{
		float m_CapsuleHalfHeight;
		u8    m_Pad[12] = {};
	public:
		phBoundCapsule() : phBoundCapsule(VEC_ORIGIN, 1.0f, 1.0f) {}
		phBoundCapsule(const Vec3V& center, float radius, float halfHeight);
		phBoundCapsule(const datResource& rsc);

		void CalculateExtents() override;

		float GetHalfLenght() const;

		float GetHalfHeight() const { return m_CapsuleHalfHeight; }
		float GetRadius() const { return m_Margin; }
	};
}
