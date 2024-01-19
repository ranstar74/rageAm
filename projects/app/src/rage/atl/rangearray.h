//
// File: rangearray.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/asserts.h"

namespace rage
{
	/**
	 * \brief C array with range check, helps to prevent array overruns.
	 */
	template<typename T, int Capacity>
	class atRangeArray
	{
		T m_Items[Capacity];

		void VerifyIdx(int i) const
		{
			AM_ASSERT(i < Capacity, "atRangeArray -> Index %i is out of bounds! Array size: %i", i, Capacity);
		}

	public:
		atRangeArray() = default;

		int GetCapacity() const { return Capacity; }

		T& operator[](int i) { VerifyIdx(i); return m_Items[i]; }
		T& begin() { return m_Items; }
		T& end () { return m_Items + Capacity; }
	};
}
