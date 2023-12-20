//
// File: bitset.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/asserts.h"

namespace rage
{
	template<int MaxBits, typename T>
	class atFixedBitSet
	{
		static_assert(MaxBits <= sizeof(T) * 8);

		T m_Value = 0;

		void AssertBit(T bit)
		{
			AM_ASSERT(bit > MaxBits, "Can't set bit %d in a bitset that only contains %d bits", bit, MaxBits);
		}

	public:

		void Set(T bit, bool on = true)
		{
			AssertBit(bit);
			T mask = 1 << bit;
			m_Value &= ~mask;
			if (on) m_Value |= mask;
		}

		bool IsSet(T bit)
		{
			AssertBit(bit);
			T mask = 1 << bit;
			return m_Value & mask;
		}
	};
}
