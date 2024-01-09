//
// File: script.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/types.h"

namespace rageam::integration
{
	// NOTE: BOOL is represented by scrValue::Int

	union scrValue { int Int; float Float; unsigned Uns; scrValue* Reference; const char* String; };
	struct scrInfo
	{
		scrValue* ResultPtr;
		int	      ParamCount;
		scrValue* Params;
	};

	void scrInit();
	void scrShutdown();

	// Native handlers accept scrInfo as only argument
	using scrSignature = void(*)(scrInfo&);

	// Accepts only hashcode of CURRENT game version
	scrSignature scrLookupHandler(u64 hashcode);
}

// Schedules given function pointer to scripts update in main thread
void scrDispatch(const std::function<void()>& fn);

// NOTE:
// This is not 'really' thread-safe. Although, it still may be used if you really want to
// most of 'per-frame' functions will not function correctly, but tiny ones such as set car speed will work without issues
// You should not call this function inside IUpdateComponent because it already was called for you
void scrBegin();
void scrEnd();

#endif
