//
// File: bvh.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "geometry.h"

namespace rage
{
	class phBoundBVH : public phBoundGeometry
	{
	public:
		phBoundBVH()
		{
			m_Type = PH_BOUND_BVH;
		}

		phBoundBVH(const datResource& rsc) : phBoundGeometry(rsc)
		{
			
		}
	};
}
