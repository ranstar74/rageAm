//
// File: com.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "helpers/macro.h"

#define SAFE_RELEASE(obj) if(obj) { (obj)->Release(); (obj) = nullptr; } MACRO_END
#define SAFE_ADDREF(obj) if(obj) (obj)->AddRef();
