//
// File: drawbucket.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/paging/resource.h"

namespace rage
{
	/**
	 * \brief In reality this is game-defined class, but for simplicity we move it here.
	 */
	enum grcRenderBucketTypes
	{
		RB_OPAQUE                      = 0,
		RB_ALPHA                       = 1,
		RB_DECAL                       = 2,
		RB_CUTOUT                      = 3,
		RB_NOSPLASH                    = 4,
		RB_NOWATER                     = 5,
		RB_WATER                       = 6,
		RB_DISPLACEMENT_ALPHA          = 7,
		RB_NUM_BASE_BUCKETS            = 8,
		RB_BASE_BUCKETS_MASK		   = 1 << RB_NUM_BASE_BUCKETS - 1,

		RB_MODEL_DEFAULT               = 8,  // Visibility
		RB_MODEL_SHADOW                = 9,
		RB_MODEL_REFLECTION_PARABOLOID = 10,
		RB_MODEL_REFLECTION_MIRROR     = 11,
		RB_MODEL_REFLECTION_WATER      = 12,

		RB_MODEL_TESSELLATION          = 15,

		RB_MODEL_OUTLINE			   = 31, // rageAm extension
	};

	/**
	 * \brief Draw / Render mask is used to effectively render only models that we are need.
	 * \n For example we can create 'Target' draw mask with RB_MODEL_REFLECTION_MIRROR
	 * flag set and easily cull out models that don't want to draw in mirror.
	 */
	struct grcDrawMask
	{
		static constexpr u32 BASE_BUCKET_MASK = 0xFF;   // (1 << RB_NUM_BASE_BUCKETS) - 1
		static constexpr u32 SUB_BUCKET_MASK  = 0xFF00; // BASE_BUCKET_MASK << RB_NUM_BASE_BUCKETS
		static constexpr u32 TESSELLATION_BIT = 1 << RB_MODEL_TESSELLATION;

		u32 Mask;

		grcDrawMask()
		{
			if (!datResource::IsBuilding())
			{
				// Default render mask is unspecified draw bucket & all other options toggled on
				Mask = 0xFFFFFF00;
			}
		}
		grcDrawMask(u32 mask) { Mask = mask; }

		u32  GetDrawBucket() const { return BitScanR32(Mask & BASE_BUCKET_MASK); }
		void SetDrawBucket(u32 bucket)
		{
			Mask &= ~BASE_BUCKET_MASK;
			Mask |= 1 << bucket;
		}

		bool GetTesellated() const { return Mask & TESSELLATION_BIT; }
		void SetTessellated(bool toggle)
		{
			Mask &= ~TESSELLATION_BIT;
			if (toggle) Mask |= TESSELLATION_BIT;
		}

		bool IsFlagSet(grcRenderBucketTypes flag) const { return Mask & 1 << flag; }
		void SetFlag(grcRenderBucketTypes flag) { Mask |= 1 << flag; }

		// Checks if this render mask satisfies target
		bool Match(grcDrawMask passed) const
		{
			return ((Mask & passed) >> RB_NUM_BASE_BUCKETS) && (Mask & BASE_BUCKET_MASK & passed);
		}

		operator u32() const { return Mask; }
	};
}
