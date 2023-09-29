//
// File: bits.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>

#include "common/types.h"

inline u8 BitScanR64(u64 value)
{
	unsigned long maxPageBit;
	_BitScanReverse64(&maxPageBit, value);
	return static_cast<u8>(maxPageBit);
}

inline u8 BitScanR32(u32 value)
{
	unsigned long maxPageBit;
	_BitScanReverse(&maxPageBit, value);
	return static_cast<u8>(maxPageBit);
}
