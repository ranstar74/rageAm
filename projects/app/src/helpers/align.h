//
// File: align.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "helpers/bits.h"

#define ALIGN_16(value) (((value) + 15) & ~15)
#define ALIGN_32(value) (((value) + 31) & ~31)
#define ALIGN_4096(value) (((value) + 4095) & ~4095)
#define ALIGN(value, align) (((value) + (align) - 1) & ~((align) - 1))

#define ALIGN_POWER_OF_TWO_64(value) (u64)(IS_POWER_OF_TWO((value)) ? (value) : 1ull << (BitScanR64((value)) + 1))
#define ALIGN_POWER_OF_TWO_32(value) (u32)(IS_POWER_OF_TWO((value)) ? (value) : 1u << (BitScanR32((value)) + 1))

#define IS_ALIGNED(value, align) ((((u64)(align) - 1) & (u64)(value)) == 0)

#define IS_POWER_OF_TWO(value) ((value) != 0 && ((value) & ((value) - 1)) == 0)

#define NEXT_POWER_OF_TWO_32(value) ((value) < 2 ? 2 : ALIGN_POWER_OF_TWO_32((value) + 1))
#define NEXT_POWER_OF_TWO_64(value) ((value) < 2 ? 2 : ALIGN_POWER_OF_TWO_64((value) + 1))
