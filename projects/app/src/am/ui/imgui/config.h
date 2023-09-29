//
// File: config.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "helpers/compiler.h"

namespace rageam
{
	AM_NOINLINE void ImAssertHandler(bool expression, ConstString assert);
}

#define IM_ASSERT(expr) rageam::ImAssertHandler(expr, #expr)
