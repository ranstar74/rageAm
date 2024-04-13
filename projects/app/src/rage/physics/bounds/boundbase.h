//
// File: boundbase.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/paging/base.h"
#include "rage/paging/place.h"
#include "rage/paging/resource.h"
#include "rage/paging/template/array.h"
#include "rage/physics/collisionflags.h"
#include "rage/physics/material.h"
#include "rage/spd/aabb.h"

namespace rage
{
	class phOptimizedBvh;

	static constexpr int BOUND_PARTS_ALL = -1;

	enum phBoundType : u8
	{
		PH_BOUND_SPHERE    = 0,
		PH_BOUND_CAPSULE   = 1,
		PH_BOUND_BOX       = 3,
		PH_BOUND_GEOMETRY  = 4,
		PH_BOUND_BVH       = 8,
		PH_BOUND_COMPOSITE = 10,
		PH_BOUND_DISC      = 12,
		PH_BOUND_CYLINDER  = 13,
		PH_BOUND_PLANE     = 15,

		PH_BOUND_UNKNOWN   = 255,
	};
	static constexpr ConstString phBoundTypeName[]
	{
		// There are excluded dummy entries because V doesn't use all supported by rage bound types
		"Sphere", "Capsule", "-", "Box", "Geometry", "-", "-", "-", "BVH", "-", "Composite", "-", "Disk", "Cylinder", "-", "Plane"
	};

	static constexpr float PH_DEFAULT_MARGIN = 0.04f;
	static constexpr float PH_BVH_MARGIN = 0.005f;

	/**
	 * \brief Base class of physics collider bound.
	 */
	class phBound : public pgBase
	{
	protected:
		phBoundType			m_Type;
		u8					m_Flags;
		u16					m_PartIndex;
		float				m_RadiusAroundCentroid;
		alignas(16) Vector3	m_BoundingBoxMax;
		float				m_Margin;
		Vector3				m_BoundingBoxMin;
		u32					m_RefCount;
		Vector3				m_CentroidOffset;
		u32					m_MaterialID0;
		Vector3				m_CGOffset;
		u32					m_MaterialID1;
		Vec4V				m_VolumeDistribution; // W is volume

	public:
		phBound();
		phBound(const datResource& rsc);

		Vec3V ComputeAngularInertia(float mass) const;
		float GetVolume() const;
		void  SetCGOffset(const Vec3V& offset);

		spdAABB   GetBoundingBox() const { return spdAABB(m_BoundingBoxMin, m_BoundingBoxMax); }
		spdSphere GetBoundingSphere() const { return spdSphere(m_CentroidOffset, m_RadiusAroundCentroid); }
		Vec3V	  GetCentroidOffset() const { return m_CentroidOffset; }
		Vec3V	  GetCGOffset() const { return m_CGOffset; }

		phMaterialMgr::Id GetPrimitiveMaterialId() const;
		void              SetPrimitiveMaterialId(phMaterialMgr::Id id);

		// Only BVH & Composite bounds can have octree (however it may be NULL if there was not enough children to generate them)
		phOptimizedBvh* GetBVH() const;

		virtual void PostLoadCompute() { }

		virtual void SetCentroidOffset(const Vec3V& offset);
		virtual void ShiftCentroidOffset(const Vec3V& offset);

		// Component is a bound in composite or octree in a BVH
		// Part is primitive element in a bound, for example - polygon, sphere, box

		virtual int				  GetNumMaterials() const { return 1; /* Primitive bounds only have single material */ }
		virtual phMaterial*		  GetMaterial(int partIndex) const { return phMaterialMgr::GetInstance()->GetDefaultMaterial(); }
		virtual void			  SetMaterial(phMaterialMgr::Id materialId, int partIndex = BOUND_PARTS_ALL) {}
		virtual phMaterialMgr::Id GetMaterialIdFromPartIndexAndComponent(int partIndex, int componentIndex = BOUND_PARTS_ALL) const = 0;

		virtual bool IsPolygonal() const { return false; }
		virtual bool CanBecomeActive() const { return true; }

		virtual void	 Copy(const phBound* from);
		virtual phBound* Clone() const;

		virtual phBoundType GetShapeType() const { return m_Type; }
		virtual ConstString GetName() const { return "RAGEBOUND"; }

	private:
		virtual void DeclareStruct() {}
		virtual void Load_V110() {}
		virtual void Save_V110() {}
	public:

		virtual void CalculateExtents() = 0;

		static phBound* Place(const datResource& rsc, phBound* that);
		static phBound* CreateOfType(phBoundType type);

		IMPLEMENT_REF_COUNTER(phBound);
	};
	using phBoundPtr = pgPtr<phBound>;
}
