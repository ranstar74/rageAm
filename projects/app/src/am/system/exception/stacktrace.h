//
// File: stacktrace.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>

#include "handler.h"
#include "common/types.h"
#include "helpers/compiler.h"
#include "rage/system/ipc/criticalsection.h"

/*
 *	=== STACK TRACER ===
 *
 * NOTE:
 *	Every member function that participates in internal call stack must have AM_NOINLINE macro!
 *	Otherwise we wouldn't be able to calculate exact number of frames to skip.
 *
*/

// TODO: Move to wchar_t

namespace rageam
{
	struct ThreadContext;
	class Thread;

	static constexpr u32 STACKTRACE_BUFFER_SIZE = 1 << 14;
	static constexpr u32 STACKTRACE_MAX_FRAMES = 64;

	/**
	 * \brief Provides functions for resolving symbols from project database file.
	 * \remarks This class is not thread-safe!
	 */
	class SymbolResolver
	{
	public:
		/**
		 * \brief This is where resolved symbol is copied to.
		 */
		struct Info
		{
			static constexpr u32 MAX_SYMBOL_NAME = 1024;
			static constexpr u32 MAX_MODULE_NAME = MAX_PATH;
			static constexpr u32 MAX_FILE_NAME = MAX_PATH;

			bool HasName;
			bool HasFileName;

			char Name[MAX_SYMBOL_NAME];
			char ModuleName[MAX_MODULE_NAME];
			wchar_t FileName[MAX_FILE_NAME];

			u32 LineNumber;
			u32 Offset;
		};

		// Force-loads symbols if they weren't loaded already.
		static void Load();
		// Force-unloads symbols if they are loaded.
		static void Free();
		// Resolves symbol at given module address.
		static void Resolve(u64 addr, Info& info);
	private:
		static inline bool sm_Loaded = false;
		// We have to store original process because WinDbg uses it as key for
		// symbol cache, and with DLL injection GetCurrentProcess is quite bugged.
		static inline HANDLE sm_Process = NULL;

		static bool ResolveSymbol(u64 addr, Info& info);
		static bool ResolveLine(u64 addr, Info& info);
	};

	/**
	 * \brief Provides functions for stack walking and writing resolved trace to buffer.
	 */
	class StackTracer
	{
		static inline u32 sm_UseCount = 0;
		static inline u64 sm_Frames[STACKTRACE_MAX_FRAMES];

		static rage::sysCriticalSectionToken sm_Mutex;

		static u16 CaptureCurrent(u32 frameSkip);
		static u16 CaptureFromContext(ExceptionHandler::Context& context);

		static inline char* sm_Buffer;
		static inline u32 sm_BufferSize;

		static void AppendToBuffer(ConstString fmt, ...);

		AM_NOINLINE static void WriteTo(u16 frameCount, char* buffer, u32 bufferSize);
	public:
		/**
		 * \brief Increments use counter and if symbols are not loaded yet, loads them.
		 * \remark This function is invoked automatically by internally from CaptureAndWriteTo overloads,
		 * and has to be used only when long-term use is required (i.e. tracing stack every frame).
		 */
		static void RequestSymbols()
		{
			rage::sysCriticalSectionLock lock(sm_Mutex);

			if (++sm_UseCount == 1)
				SymbolResolver::Load();
		}

		/**
		 * \brief Decrements use counter and if there's no references remaining, unloads symbols from memory.
		 */
		static void FreeSymbols()
		{
			rage::sysCriticalSectionLock lock(sm_Mutex);

			if (--sm_UseCount == 0)
				SymbolResolver::Free();
		}

		static void Shutdown()
		{
			SymbolResolver::Free();
		}

		/**
		 * \brief Traces current thread stack and writes traced stack with resolved symbol names to buffer.
		 * \param buffer		Buffer into which stack will be printed.
		 * \param bufferSize	Size of the buffer. If buffer is not large enough to fit stack trace, exception is thrown.
		 * \param frameSkip		Number of stack frames to skip. Leaving it to 0 will trace all the way from call address,
		 *	but if call function is internal and should't appear in stack trace, it can be skipped.
		 */
		AM_NOINLINE static void CaptureAndWriteTo(char* buffer, u32 bufferSize, u32 frameSkip = 0)
		{
			rage::sysCriticalSectionLock lock(sm_Mutex);
			RequestSymbols();
			WriteTo(CaptureCurrent(frameSkip + 1 /* This */), buffer, bufferSize);
			FreeSymbols();
		}

		/**
		 * \brief Traces stack from execution context and writes traced stack with resolved symbol names to buffer.
		 * \param context		Context from exception filter.
		 * \param buffer		Buffer into which stack will be printed.
		 * \param bufferSize	Size of the buffer. If buffer is not large enough to fit stack trace, exception is thrown.
		 */
		AM_NOINLINE static void CaptureAndWriteTo(ExceptionHandler::Context& context, char* buffer, u32 bufferSize)
		{
			rage::sysCriticalSectionLock lock(sm_Mutex);
			RequestSymbols();
			WriteTo(CaptureFromContext(context), buffer, bufferSize);
			FreeSymbols();
		}
	};
}
