//
// File: macro.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#define MACRO_END \
	static_assert(true, "" /* To require semicolon after macro */)

#ifdef _DEBUG
#define AM_DEBUG_ONLY(action) action
#else
#define AM_DEBUG_ONLY(action) 
#endif

#ifdef AM_INTEGRATED
#define AM_INTEGRATED_ONLY(action) action
#else
#define AM_INTEGRATED_ONLY(action)
#endif

#ifdef AM_STANDALONE
#define AM_STANDALONE_ONLY(action) action
#else
#define AM_STANDALONE_ONLY(action)
#endif
