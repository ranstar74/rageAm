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
#define ASSERT_MAX 256

	WPRINTF_ATTR(5, 6) AM_NOINLINE	void AssertHandler(bool expression, ConstString assert, ConstString file, int line, ConstWString fmt, ...);
	PRINTF_ATTR(5, 6) AM_NOINLINE	void AssertHandler(bool expression, ConstString assert, ConstString file, int line, ConstString fmt, ...);
	PRINTF_ATTR(5, 6) AM_NOINLINE	bool VerifyHandler(bool expression, ConstString assert, ConstString file, int line, ConstWString fmt, ...);
	PRINTF_ATTR(5, 6) AM_NOINLINE	bool VerifyHandler(bool expression, ConstString assert, ConstString file, int line, ConstString fmt, ...);
	PRINTF_ATTR(3, 4) AM_NORET		void Unreachable(ConstString file, int line, ConstWString fmt, ...);
	PRINTF_ATTR(3, 4) AM_NORET		void Unreachable(ConstString file, int line, ConstString fmt, ...);
}

#ifndef AM_UNIT_TESTS
#define AM_ASSERT(expr, fmt, ...)	rageam::AssertHandler(expr, #expr, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define AM_VERIFY(expr, fmt, ...)	rageam::VerifyHandler(expr, #expr, __FILE__, __LINE__, fmt, __VA_ARGS__)
#define AM_UNREACHABLE(fmt, ...)	rageam::Unreachable(__FILE__, __LINE__, fmt, __VA_ARGS__)
#else

namespace unit_testing
{
	WPRINTF_ATTR(2, 3) AM_NOINLINE	bool AssertHandler(bool expression, const wchar_t* fmt, ...);
	PRINTF_ATTR(2, 3) AM_NOINLINE	bool AssertHandler(bool expression, const char* fmt, ...);
}

#define AM_ASSERT(expr, fmt, ...)	unit_testing::AssertHandler(expr, fmt, __VA_ARGS__)
#define AM_VERIFY(expr, fmt, ...)	unit_testing::AssertHandler(expr, fmt, __VA_ARGS__ /* We take everything seriously in unit testing */ )
#define AM_UNREACHABLE(fmt, ...)	unit_testing::AssertHandler(false, fmt, __VA_ARGS__); std::exit(-1)
#endif
