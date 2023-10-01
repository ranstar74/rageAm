//
// File: boundbase.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/paging/base.h"
#include "rage/paging/place.h"
#include "rage/paging/resource.h"
#include "rage/paging/template/array.h"
#include "rage/spd/aabb.h"
#include "rage/physics/collisionflags.h"
#include "rage/physics/material.h"

namespace rage
{
	static constexpr int BOUND_PARTS_ALL = -1;

	enum phBoundMaterialFlags
	{
		BMF_NONE = 0,
		BMF_STAIRS = 1 << 0,
		BMF_NOT_CLIMBABLE = 1 << 1,
		BMF_SEE_THROUGH = 1 << 2,
		BMF_SHOOT_THROUGH = 1 << 3,
		BMF_NOT_COVER = 1 << 4,
		BMF_WALKABLE_PATH = 1 << 5,
		BMF_NO_CAM_COLLISION = 1 << 6,
		BMF_SHOOT_THROUGH_FX = 1 << 7,
		BMF_NO_DECAL = 1 << 8,
		BMF_NO_NAVMESH = 1 << 9,
		BMF_NO_RAGDOLL = 1 << 10,
		BMF_VEHICLE_WHEEL = 1 << 11,
		BMF_NO_PTFX = 1 << 12,
		BMF_TOO_STEEP_FOR_PLAYER = 1 << 13,
		BMF_NO_NETWORK_SPAWN = 1 << 14,
		BMF_NO_CAM_COLLISION_ALLOW_CLIPPING = 1 << 15,
	};

	// Only for bound primitives, geometry bound stores flags per poly
	inline u64 phApplyFlagsToMaterialID(u64 materialID, phBoundMaterialFlags flags)
	{
		return materialID | flags << 24;
	}

	inline phBoundMaterialFlags phGetFlagsFromMaterialID(u64 materialID)
	{
		return phBoundMaterialFlags(materialID >> 24 & 0xF);
	}

	enum phBoundType : u8
	{
		PH_BOUND_SPHERE = 0,
		PH_BOUND_CAPSULE = 1,
		PH_BOUND_BOX = 3,
		PH_BOUND_GEOMETRY = 4,
		PH_BOUND_BVH = 8,
		PH_BOUND_COMPOSITE = 10,
		PH_BOUND_DISC = 12,
		PH_BOUND_CYLINDER = 13,
		PH_BOUND_PLANE = 15,

		PH_BOUND_UNKNOWN = 255,
	};

	static constexpr float PH_DEFAULT_MARGIN = 0.04f;

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

		// ReSharper disable once CppPossiblyUninitializedMember
		phBound(const datResource& rsc)
		{

		}

		Vec3V ComputeAngularInertia(float mass) const;
		float GetVolume() const;
		void SetCGOffset(const Vec3V& offset);

		spdAABB GetBoundingBox() const { return spdAABB(m_BoundingBoxMin, m_BoundingBoxMax); }

		virtual void PostLoadCompute() { }

		virtual void SetCentroidOffset(const Vec3V& offset);
		virtual void ShiftCentroidOffset(const Vec3V& offset);

		virtual u8 GetNumMaterials() const { return 1; /* Primitive bounds only have single material */ }
		virtual phMaterial* GetMaterial(int partIndex) const;
		virtual void SetMaterial(u64 materialId, int partIndex = BOUND_PARTS_ALL);
		// Bound index is used only on composite
		virtual u64 GetMaterialIdFromPartIndex(int partIndex, int boundIndex = -1) const = 0;

		virtual bool IsPolygonal() const { return false; }
		virtual bool CanBecomeActive() const { return true; }

		virtual void Copy(const phBound* from);
		virtual phBound* Clone() const;

		virtual phBoundType GetShapeType() const { return m_Type; }
		virtual ConstString GetName() const { return "RAGEBOUND"; }

		// Parses bound from text file. Unused in release build, we don't need it
		virtual void Load() {}

		virtual void CalculateExtents() = 0;

		static phBound* Place(const datResource& rsc, phBound* that);
		static phBound* CreateOfType(phBoundType type);

		IMPLEMENT_REF_COUNTER(phBound);
	};
	using phBoundPtr = pgPtr<phBound>;
}
