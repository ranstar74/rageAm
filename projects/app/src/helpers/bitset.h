//
// File: bitset.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

template<typename T>
struct BitSet
{
	T Value = 0;

	void SetBit(u8 bit, bool on)
	{
		Value &= ~(1 << bit);
		Value |= on << bit;
	}

	bool IsBitSet(u8 bit) const
	{
		return Value >> bit & 1;
	}
};

typedef BitSet<u32> BitSet32;
typedef BitSet<u64> BitSet64;
