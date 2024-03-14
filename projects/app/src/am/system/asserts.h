//
// File: asserts.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "helpers/compiler.h"
#include "helpers/resharper.h"

// #define ENABLE_VERIFY_STACKTRACE

namespace rageam
{
	// TODO: This and errordisplay.h have some insane amount of code duplicate

	// Max length of assert/verify message buffer
#define ASSERT_MAX 1024

	WPRINTF_ATTR(5, 6) AM_NOINLINE	void AssertHandler(bool expression, ConstString assert, ConstString file, int line, ConstWString fmt, ...);
	PRINTF_ATTR(5, 6) AM_NOINLINE	void AssertHandler(bool expression, ConstString assert, ConstString file, int line, ConstString fmt, ...);
	PRINTF_ATTR(5, 6) AM_NOINLINE	bool VerifyHandler(bool expression, ConstString assert, ConstString file, int line, ConstWString fmt, ...);
	PRINTF_ATTR(5, 6) AM_NOINLINE	bool VerifyHandler(bool expression, ConstString assert, ConstString file, int line, ConstString fmt, ...);
	PRINTF_ATTR(3, 4) AM_NORET		void Unreachable(ConstString file, int line, ConstWString fmt, ...);
	PRINTF_ATTR(3, 4) AM_NORET		void Unreachable(ConstString file, int line, ConstString fmt, ...);
}

#ifndef AM_UNIT_TESTS
#ifdef DEBUG
#define AM_DEBUG_ASSERT(expr, fmt, ...)	rageam::AssertHandler(expr, #expr, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define AM_DEBUG_ASSERTS(expr)			rageam::AssertHandler(expr, #expr, __FILE__, __LINE__, "")
#else
#define AM_DEBUG_ASSERT(expr, fmt, ...)
#define AM_DEBUG_ASSERTS(expr)
#endif // DEBUG
#define AM_ASSERTS(expr, fmt, ...)		rageam::AssertHandler(expr, #expr, __FILE__, __LINE__, "")
#define AM_ASSERT(expr, fmt, ...)		rageam::AssertHandler(expr, #expr, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define AM_VERIFY(expr, fmt, ...)		rageam::VerifyHandler(expr, #expr, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define AM_UNREACHABLE(fmt, ...)		rageam::Unreachable(__FILE__, __LINE__, fmt, __VA_ARGS__)
#else // NOT AM_UNIT_TESTS

namespace unit_testing
{
	WPRINTF_ATTR(2, 3) AM_NOINLINE	bool AssertHandler(bool expression, const wchar_t* fmt, ...);
	PRINTF_ATTR(2, 3) AM_NOINLINE	bool AssertHandler(bool expression, const char* fmt, ...);
}
#include <cstdlib> // std::exit
#define AM_DEBUG_ASSERT(expr, fmt, ...)	unit_testing::AssertHandler(expr, fmt, __VA_ARGS__)
#define AM_DEBUG_ASSERTS(expr)			unit_testing::AssertHandler(expr, "")
#define AM_ASSERTS(expr)				unit_testing::AssertHandler(expr, "")
#define AM_ASSERT(expr, fmt, ...)		unit_testing::AssertHandler(expr, fmt, __VA_ARGS__)
#define AM_VERIFY(expr, fmt, ...)		unit_testing::AssertHandler(expr, fmt, __VA_ARGS__ /* We take everything seriously in unit testing */ )
#define AM_UNREACHABLE(fmt, ...)		unit_testing::AssertHandler(false, fmt, __VA_ARGS__); std::exit(-1)
#endif // AM_UNIT_TESTS

#define AM_ASSERT_STATUSF(expr, fmt, ...)		AM_ASSERT((expr) == S_OK, fmt, __VA_ARGS__)
#define AM_ASSERT_STATUS(expr)					AM_ASSERTS((expr) == S_OK)
