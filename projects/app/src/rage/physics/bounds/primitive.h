//
// File: primitive.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "boundbase.h"

namespace rage
{
	/**
	 * \brief This non-standard class was added because all primitive bounds share GetMaterialID virtual function,
	 * but it's not part of phBound virtual table.
	 */
	class phBoundPrimitive : public phBound
	{
	public:
		phBoundPrimitive() = default;
		phBoundPrimitive(const datResource& rsc) : phBound(rsc) {}

		u64 GetMaterialIdFromPartIndex(int partIndex, int boundIndex = -1) const override { return GetMaterialID(); }

		virtual u64 GetMaterialID() const { return m_MaterialID0 | u64(m_MaterialID1) << 32; }
	};
}
