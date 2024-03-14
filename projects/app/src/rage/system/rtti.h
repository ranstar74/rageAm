//
// File: rtti.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/atl/hashstring.h"
#include "am/system/asserts.h"

namespace rage
{
	// In game update 2802 R* removed native C++ RTTI system (including the names)
	// and implemented their own system
	// Every rage/game class with virtual function table must define following macro
	// in the beginning of definition (even before virtual destructor):
	// class pgBase
	// {
	//     DECLARE_RTTI();
	//     ...

	// NOTE: This new system is not used in all classes!
	// It's manually defined only in required ones, for example rage::grcTextureFactory don't have it.

	// Those functions are not researched well and not used often,
	// for now use placeholder...

#ifdef USE_RAGE_RTTI
#define DECLARE_RTTI(type) \
	virtual void*		TypeInfo_Fn1()			   { AM_UNREACHABLE("TypeInfo_Fn1 -> Not implemented."); } \
	virtual void*		TypeInfo_Fn2()			   { AM_UNREACHABLE("TypeInfo_Fn2 -> Not implemented."); } \
	virtual atHashValue TypeInfo_GetBaseNameHash() { return atStringHash(#type); } \
	virtual type*		TypeInfo_Fn4(void*)		   { AM_UNREACHABLE("TypeInfo_Fn3 -> Not implemented."); } \
	virtual bool		TypeInfo_Fn5(void*)		   { AM_UNREACHABLE("TypeInfo_Fn4 -> Not implemented."); } \
	virtual bool		TypeInfo_Fn6(void**)	   { AM_UNREACHABLE("TypeInfo_Fn5 -> Not implemented."); } \
	static_assert(1) /* Suppress 'extra ; inside a class' warning */
#else
#define DECLARE_RTTI(type) static_assert(1)
#endif
}
