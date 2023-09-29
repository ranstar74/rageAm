//
// File: fourcc.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

#define FOURCC(ch0, ch1, ch2, ch3) \
                ((u32)(u8)(ch0) | ((u32)(u8)(ch1) << 8) | \
                ((u32)(u8)(ch2) << 16) | ((u32)(u8)(ch3) << 24 ))
