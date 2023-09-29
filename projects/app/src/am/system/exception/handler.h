//
// File: handler.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>
#include <excpt.h>

#include "common/types.h"
#include "helpers/compiler.h"

namespace rageam
{
	/**
	 * \brief Provides functionality for catching runtime exceptions in process.
	 */
	class ExceptionHandler
	{
	public:
		struct Context
		{
			u32 ExceptionCode;
			u64 ExceptionAddress;

			ConstString ExceptionName;

			CONTEXT ExecutionRecord;

			Context(const _EXCEPTION_POINTERS* exInfo);
		};
	private:
		AM_NOINLINE static void HandleException(const EXCEPTION_POINTERS* exInfo, bool isHandled);

		// ReSharper disable once CppParameterMayBeConstPtrOrRef
		AM_NOINLINE static LONG ExceptionFilter(EXCEPTION_POINTERS* exInfo);
		AM_NOINLINE static LONG ExceptionFilterSafe(const _EXCEPTION_POINTERS* exInfo);
	public:
		static void Init();
		static void Shutdown();

		/**
		 * \brief Alternative to 'try-catch', catches any possible runtime exception.
		 * \n If debugger is attached, debug break is called; Otherwise dialog with exception is shown.
		 * \n Stack trace is printed to general log file in either of both cases.
		 */
		template<typename TFunc, typename ... Args>
		static bool ExecuteSafe(TFunc action, Args ... args);
	};
}

template <typename TFunc, typename ... Args>
bool rageam::ExceptionHandler::ExecuteSafe(TFunc action, Args... args)
{
	__try
	{
		action(args...);
	}
	__except (ExceptionFilterSafe(GetExceptionInformation()))
	{
		return false;
	}
	return true;
}
