//
// File: ranges.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

#define DISTANCE(lhs, rhs) ((u64)(rhs) - (u64)(lhs))

#define MAX(lhs, rhs) ((lhs) > (rhs) ? (lhs) : (rhs))
#define MIN(lhs, rhs) ((lhs) > (rhs) ? (rhs) : (lhs))

// Gets whether given pointer is within the block of given size
#define IS_WITHIN(ptr, block, blockSize) ((u64)(ptr) >= (u64)(block) && (u64)(ptr) < ((u64)(block) + (blockSize)))

// Size of a static C-style array. Don't use on pointers!
#define ARRAYSIZE(_ARR) ((int)(sizeof(_ARR) / sizeof(*(_ARR))))
