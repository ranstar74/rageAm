//
// File: composite.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "boundbase.h"
#include "rage/math/mtxv.h"
#include "rage/physics/optimizedbvh.h"

// TODO: BHV, Copy / Clone

namespace rage
{
	/**
	 * \brief A group of bounds.
	 * Allows to set transformation matrix for child bounds (such as axis aligned bounding box) and collision flags.
	 */
	class phBoundComposite : public phBound
	{
		struct CompositeFlags
		{
			CollisionFlags Type;
			CollisionFlags Include;
		};

		pgCArray<phBoundPtr>		m_Bounds;
		pgCArray<Mat44V>			m_CurrentMatrices;
		pgCArray<Mat44V>			m_LastMatrices;
		pgCArray<spdAABB>			m_AABBs;
		pgCArray<CompositeFlags>	m_TypeAndIncludeFlags;
		pgCArray<CompositeFlags>	m_OwnedTypeAndIncludeFlags; // Allocated calling AllocateTypeAndIncludeFlags, we don't need them
		u16							m_MaxNumBounds;				// Number of bounds in array
		u16							m_NumBounds;				// Number of 'active' bounds in array
		pgUPtr<phOptimizedBvh>		m_BVH;

		// Ensures that given index is less than m_MaxNumBounds
		void AssertWithinArray(u16 index) const;

	public:
		phBoundComposite();
		phBoundComposite(const datResource& rsc);
		~phBoundComposite() override;

		u16			GetNumBounds() const { return m_MaxNumBounds; }
		phBoundPtr& GetBound(u16 index);
		void		SetBound(u16 index, const phBoundPtr& bound);

		void		   SetTypeFlags(u16 index, CollisionFlags flags);
		void		   SetIncludeFlags(u16 index, CollisionFlags flags);
		CollisionFlags GetTypeFlags(u16 index);
		CollisionFlags GetIncludeFlags(u16 index);

		void		  SetMatrix(u16 index, const Mat44V& mtx);
		const Mat44V& GetMatrix(u16 index);

		void InitFromArray(const atArray<phBoundPtr>& bounds, bool allowInternalMotion);
		void Init(u16 numBounds, bool allowInternalMotion);
		bool AllowsInternalMotion() const { return m_CurrentMatrices != m_LastMatrices; }
		void MakeDynamic();

		void CalculateBoundingBox();
		void CalculateVolume();

		void PostLoadCompute() override
		{
			// TODO: BVH
		}

		void Copy(const phBound* from) override { AM_UNREACHABLE("phBoundComposite::Copy() -> Not Implemented."); }

		void ShiftCentroidOffset(const Vec3V& offset) override { /* Composite doesn't support centroid offset */ }

		int               GetNumMaterials() const override { return m_Bounds[0]->GetNumMaterials(); }
		phMaterial*       GetMaterial(int partIndex) const override { return GetMaterial(partIndex, 0); }
		phMaterialMgr::Id GetMaterialIdFromPartIndexAndComponent(int partIndex, int boundIndex) const override;

		bool CanBecomeActive() const override;
		void CalculateExtents() override;

		phOptimizedBvh* GetBVH() const { return m_BVH.Get(); }

		virtual phMaterial*		  GetMaterial(int partIndex, int boundIndex) const;
		virtual phMaterialMgr::Id GetMaterialId(int partIndex, int boundIndex) const;
		virtual phMaterialMgr::Id GetMaterialId(int partIndex) const;
	};
}
