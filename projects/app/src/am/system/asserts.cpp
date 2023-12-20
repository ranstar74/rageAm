#include "asserts.h"

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include "debugger.h"
#include "errordisplay.h"
#include "common/logger.h"

void rageam::AssertHandler(bool expression, ConstString assert, ConstString file, int line, ConstWString fmt, ...)
{
	if (expression)
		return;

	wchar_t formatBuffer[ASSERT_MAX];
	va_list args;
	va_start(args, fmt);
	vswprintf_s(formatBuffer, ASSERT_MAX, fmt, args);
	va_end(args);

	wchar_t errorBuffer[ASSERT_MAX];
	swprintf_s(errorBuffer, ASSERT_MAX, L"File: %hs at line: %i\n%ls", file, line, formatBuffer);

	ErrorDisplay::Assert(formatBuffer, assert, 1 /* This */);
	Debugger::BreakIfAttached();

	std::exit(1);
}

void rageam::AssertHandler(bool expression, ConstString assert, ConstString file, int line, ConstString fmt, ...)
{
	if (expression)
		return;

	char formatBuffer[ASSERT_MAX];

	va_list args;
	va_start(args, fmt);
	vsprintf_s(formatBuffer, ASSERT_MAX, fmt, args);
	va_end(args);

	AssertHandler(expression, assert, file, line, L"%hs", formatBuffer);
}

bool rageam::VerifyHandler(bool expression, ConstString assert, ConstString file, int line, ConstWString fmt, ...)
{
	if (expression)
		return true;

	wchar_t formatBuffer[ASSERT_MAX];
	va_list args;
	va_start(args, fmt);
	vswprintf_s(formatBuffer, ASSERT_MAX, fmt, args);
	va_end(args);

#ifdef ENABLE_VERIFY_STACKTRACE
	wchar_t errorBuffer[ASSERT_MAX];
	swprintf_s(errorBuffer, ASSERT_MAX, L"File: %hs at line: %i\n%ls", file, line, formatBuffer);

	ErrorDisplay::Verify(formatBuffer, assert, 1 /* This */);

	return false;
#else
	AM_ERR(formatBuffer);
	return false;
#endif
}

bool rageam::VerifyHandler(bool expression, ConstString assert, ConstString file, int line, ConstString fmt, ...)
{
	if (expression)
		return true;

	char formatBuffer[ASSERT_MAX];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(formatBuffer, ASSERT_MAX, fmt, args);
	va_end(args);

	return VerifyHandler(expression, assert, file, line, L"%hs", formatBuffer);
}

void rageam::Unreachable(ConstString file, int line, ConstWString fmt, ...)
{
	// https://www.youtube.com/watch?v=-G2n6UqOWIo

	wchar_t formatBuffer[ASSERT_MAX];
	va_list args;
	va_start(args, fmt);
	vswprintf_s(formatBuffer, ASSERT_MAX, fmt, args);
	va_end(args);

	wchar_t errorBuffer[ASSERT_MAX];
	swprintf_s(errorBuffer, ASSERT_MAX, L"File: %hs at line: %i\n%ls", file, line, formatBuffer);

	ErrorDisplay::Assert(errorBuffer, "", 1 /* This */);
	Debugger::BreakIfAttached();

	std::exit(1);
}

void rageam::Unreachable(ConstString file, int line, ConstString fmt, ...)
{
	// https://www.youtube.com/watch?v=-G2n6UqOWIo

	char formatBuffer[ASSERT_MAX];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(formatBuffer, ASSERT_MAX, fmt, args);
	va_end(args);

	Unreachable(file, line, L"%hs", formatBuffer);
}

#ifdef AM_UNIT_TESTS

#include "CppUnitTest.h"

bool unit_testing::AssertHandler(bool expression, const wchar_t* fmt, ...)
{
	if (expression) return true;

	wchar_t formatBuffer[ASSERT_MAX];
	va_list args;
	va_start(args, fmt);
	vswprintf_s(formatBuffer, ASSERT_MAX, fmt, args);
	va_end(args);

	Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(expression, formatBuffer);
	return false; // Unreachable
}

bool unit_testing::AssertHandler(bool expression, const char* fmt, ...)
{
	if (expression) return true;

	char formatBuffer[ASSERT_MAX];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(formatBuffer, ASSERT_MAX, fmt, args);
	va_end(args);

	return AssertHandler(expression, L"%hs", formatBuffer);
}

#endif
