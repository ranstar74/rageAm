#include "handler.h"

#include "stacktrace.h"
#include "am/system/debugger.h"
#include "am/system/errordisplay.h"

static LPTOP_LEVEL_EXCEPTION_FILTER m_PreviousFilter;

// ReSharper disable once CppDeclaratorNeverUsed
static rageam::ExceptionHandler g_ExceptionHandler;

const char* GetExceptionName(DWORD code)
{
	if (code == 0xC0000005L) return "EXCEPTION_ACCESS_VIOLATION";
	if (code == 0x80000002L) return "EXCEPTION_DATATYPE_MISALIGNMENT";
	if (code == 0x80000003L) return "EXCEPTION_BREAKPOINT";
	if (code == 0x80000004L) return "EXCEPTION_SINGLE_STEP";
	if (code == 0xC000008CL) return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
	if (code == 0xC000008DL) return "EXCEPTION_FLT_DENORMAL_OPERAND";
	if (code == 0xC000008EL) return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
	if (code == 0xC000008FL) return "EXCEPTION_FLT_INEXACT_RESULT";
	if (code == 0xC0000090L) return "EXCEPTION_FLT_INVALID_OPERATION";
	if (code == 0xC0000091L) return "EXCEPTION_FLT_OVERFLOW";
	if (code == 0xC0000092L) return "EXCEPTION_FLT_STACK_CHECK";
	if (code == 0xC0000093L) return "EXCEPTION_FLT_UNDERFLOW";
	if (code == 0xC0000094L) return "EXCEPTION_INT_DIVIDE_BY_ZERO";
	if (code == 0xC0000095L) return "EXCEPTION_INT_OVERFLOW";
	if (code == 0xC0000096L) return "EXCEPTION_PRIV_INSTRUCTION";
	if (code == 0xC0000006L) return "EXCEPTION_IN_PAGE_ERROR";
	if (code == 0xC000001DL) return "EXCEPTION_ILLEGAL_INSTRUCTION";
	if (code == 0xC0000025L) return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
	if (code == 0xC00000FDL) return "EXCEPTION_STACK_OVERFLOW";
	if (code == 0xC0000026L) return "EXCEPTION_INVALID_DISPOSITION";
	if (code == 0x80000001L) return "EXCEPTION_GUARD_PAGE";
	if (code == 0xC0000008L) return "EXCEPTION_INVALID_HANDLE";
	return "";
}

rageam::ExceptionHandler::Context::Context(const _EXCEPTION_POINTERS* exInfo)
{
	ExceptionCode = exInfo->ExceptionRecord->ExceptionCode;
	ExceptionName = GetExceptionName(ExceptionCode);
	ExceptionAddress = reinterpret_cast<u64>(exInfo->ExceptionRecord->ExceptionAddress);

	// We need a copy of execution context because StackWalk64 in
	// StackTracer::CaptureFromContext modifies it and app crashes
	// (apparently happens only with 64 bit version of that function)
	memcpy(&ExecutionRecord, exInfo->ContextRecord, sizeof CONTEXT);
}

AM_NOINLINE void rageam::ExceptionHandler::HandleException(const EXCEPTION_POINTERS* exInfo, bool isHandled)
{
	Context ctx(exInfo);
	ErrorDisplay::Exception(ctx, isHandled);
	Debugger::BreakIfAttached(); // If debugger wasn't attached, dialog gives you a great time to do it
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
AM_NOINLINE LONG rageam::ExceptionHandler::ExceptionFilter(EXCEPTION_POINTERS* exInfo)
{
	HandleException(exInfo, false);
	return EXCEPTION_CONTINUE_SEARCH;
}

AM_NOINLINE LONG rageam::ExceptionHandler::ExceptionFilterSafe(const _EXCEPTION_POINTERS* exInfo)
{
	if (Debugger::IsPresent()) // Pass exception to attached debugger
		return EXCEPTION_CONTINUE_SEARCH;

	HandleException(exInfo, true);
	return EXCEPTION_EXECUTE_HANDLER;
}

void rageam::ExceptionHandler::Init()
{
	m_PreviousFilter = SetUnhandledExceptionFilter(ExceptionFilter);
}

void rageam::ExceptionHandler::Shutdown()
{
	SetUnhandledExceptionFilter(m_PreviousFilter);

	StackTracer::Shutdown();
}

rageam::ExceptionHandler* GetExceptionHandler()
{
	return &g_ExceptionHandler;
}
