//
// File: utf8.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

// This exists because with new standard c++ uses char8u_t instead of char
#define U8(text) (const char*)u8##text
