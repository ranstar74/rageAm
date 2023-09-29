//
// File: errordisplay.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "exception/handler.h"
#include "helpers/compiler.h"

namespace rage
{
	class sysMemAllocator;
	class sysCriticalSectionToken;
}

namespace rageam
{
	/**
	 * \brief Contains methods for handling error popups/console prints.
	 */
	class ErrorDisplay
	{
		static rage::sysCriticalSectionToken sm_Mutex;

		static wchar_t sm_StackTraceBuffer[];

		AM_NOINLINE static ConstWString CaptureStack(u32 frameSkip);
	public:
		AM_NOINLINE static void OutOfMemory(rage::sysMemAllocator* allocator, u64 allocSize, u64 allocAlign);
		AM_NOINLINE static void Assert(ConstWString error, ConstString assert, u32 frameSkip);
		AM_NOINLINE static void GameError(ConstWString error, u32 frameSkip);
		AM_NOINLINE static void ImAssert(ConstString assert, u32 frameSkip);
		AM_NOINLINE static void Verify(ConstWString error, ConstString assert, u32 frameSkip);
		AM_NOINLINE static void Exception(ExceptionHandler::Context& context, bool isHandled);
	};
}
