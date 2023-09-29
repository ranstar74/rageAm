#include "errordisplay.h"

#include <vcruntime_typeinfo.h>

#include "dialog.h"
#include "am/string/string.h"
#include "common/logger.h"
#include "exception/handler.h"
#include "exception/stacktrace.h"
#include "helpers/format.h"

rage::sysCriticalSectionToken rageam::ErrorDisplay::sm_Mutex;
wchar_t rageam::ErrorDisplay::sm_StackTraceBuffer[STACKTRACE_BUFFER_SIZE];

AM_NOINLINE ConstWString rageam::ErrorDisplay::CaptureStack(u32 frameSkip)
{
	static char buffer[STACKTRACE_BUFFER_SIZE];

	StackTracer::CaptureAndWriteTo(buffer, STACKTRACE_BUFFER_SIZE, frameSkip + 1 /* This */);

	swprintf_s(sm_StackTraceBuffer, STACKTRACE_BUFFER_SIZE, L"%hs", buffer);
	return sm_StackTraceBuffer;
}

AM_NOINLINE void rageam::ErrorDisplay::OutOfMemory(rage::sysMemAllocator* allocator, u64 allocSize, u64 allocAlign)
{
	rage::sysCriticalSectionLock lock(sm_Mutex);

	char buffer[256]{};

	u64 used = allocator->GetMemoryUsed();
	u64 available = allocator->GetMemoryAvailable();
	u64 toExtend = allocSize - available;

	char requestedText[32];
	char usedText[32];
	char availableText[32];
	char toExtendText[32];
	FormatBytes(requestedText, 32, allocSize);
	FormatBytes(usedText, 32, used);
	FormatBytes(availableText, 32, available);
	FormatBytes(toExtendText, 32, toExtend);

	// We only got 3 system allocators (general / virtual / physical) so class name is enough to identify them.
	ConstString allocatorName = typeid(*allocator).name();

	sprintf_s(buffer, 256, "Allocator (%s) is out of memory!\n"
		"Available: %s\nUsed: %s\nRequested: %s w align %llu (size / align)\n\n"
		"Heap size has to be extended by at least %s.\n\n"
		"Make sure debugger is attached.",
		allocatorName, availableText, usedText, requestedText, allocAlign, toExtendText);

	FlagSet<eLogOptions>& options = Logger::GetInstance()->GetOptions();
	options.Set(LOG_OPTION_NO_PREFIX, true);
	AM_ERR("");
	AM_ERR("=== OUT OF MEMORY ===");
	AM_ERR(buffer);
	AM_TRACE(CaptureStack(1 /* This */));
	options.Set(LOG_OPTION_NO_PREFIX, false);

	ShowDialog("Out Of Memory", buffer, DIALOG_ERROR);
}

AM_NOINLINE void rageam::ErrorDisplay::Assert(ConstWString error, ConstString assert, u32 frameSkip)
{
	rage::sysCriticalSectionLock lock(sm_Mutex);

	ConstWString stack = CaptureStack(frameSkip + 1 /* This */);

	// Print assert to console
	FlagSet<eLogOptions>& options = Logger::GetInstance()->GetOptions();
	options.Set(LOG_OPTION_NO_PREFIX, true);
	AM_ERR("\n=== ASSERTION FAILED ===");
	AM_ERRF(L"%hs, %ls", assert, error);
	AM_TRACE(stack);
	options.Set(LOG_OPTION_NO_PREFIX, false);

	// Show task dialog with assert
	TASKDIALOG_BUTTON buttons[1];
	buttons[0].nButtonID = 0;
	buttons[0].pszButtonText = L"Break Debugger / Exit";

	wchar_t assertw[ASSERT_MAX];
	String::ToWide(assertw, ASSERT_MAX, assert);

	ShowTaskDialog(L"Assertion Failed", L"Make sure debugger is attached.",
		error, assertw, stack, DIALOG_ERROR, buttons, 1);
}

void rageam::ErrorDisplay::GameError(ConstWString error, u32 frameSkip)
{
	rage::sysCriticalSectionLock lock(sm_Mutex);

	ConstWString stack = CaptureStack(frameSkip + 1 /* This */);

	auto& options = rageam::Logger::GetInstance()->GetOptions();
	options.Set(LOG_OPTION_NO_PREFIX, true);
	AM_ERR("\n=== GAME ERROR ===");
	AM_ERRF("%ls", error);
	AM_TRACE(stack);
	options.Set(LOG_OPTION_NO_PREFIX, false);

	TASKDIALOG_BUTTON buttons[1];
	buttons[0].nButtonID = 0;
	buttons[0].pszButtonText = L"Break Debugger / Exit";

	ShowTaskDialog(L"Game Error", L"Make sure debugger is attached",
		error, L"", stack, DIALOG_ERROR, buttons, 1);
}

void rageam::ErrorDisplay::ImAssert(ConstString assert, u32 frameSkip)
{
	rage::sysCriticalSectionLock lock(sm_Mutex);

	ConstWString stack = CaptureStack(frameSkip + 1 /* This */);

	auto& options = Logger::GetInstance()->GetOptions();
	options.Set(LOG_OPTION_NO_PREFIX, true);
	AM_ERR("\n=== IM GUI ASSERTION FAILED ===");
	AM_ERRF("%s", assert);
	AM_TRACE(stack);
	options.Set(LOG_OPTION_NO_PREFIX, false);

	TASKDIALOG_BUTTON buttons[1];
	buttons[0].nButtonID = 0;
	buttons[0].pszButtonText = L"Break Debugger / Exit";

	wchar_t assertw[ASSERT_MAX];
	String::ToWide(assertw, ASSERT_MAX, assert);

	ShowTaskDialog(L"Assertion Failed", L"Make sure debugger is attached.",
		L"ImGui Assert", assertw, stack, DIALOG_ERROR, buttons, 1);
}

AM_NOINLINE void rageam::ErrorDisplay::Verify(ConstWString error, ConstString assert, u32 frameSkip)
{
	rage::sysCriticalSectionLock lock(sm_Mutex);

	AM_WARNINGF(L"\nVerify failed: %hs, %ls", assert, error);
	AM_TRACE(CaptureStack(frameSkip + 1 /* This */));
}

AM_NOINLINE void rageam::ErrorDisplay::Exception(rageam::ExceptionHandler::Context& context, bool isHandled)
{
	rage::sysCriticalSectionLock lock(sm_Mutex);

	// Capture exception stack trace & convert it to wide
	static char stackTrace[STACKTRACE_BUFFER_SIZE];
	StackTracer::CaptureAndWriteTo(context, stackTrace, STACKTRACE_BUFFER_SIZE);
	swprintf_s(sm_StackTraceBuffer, STACKTRACE_BUFFER_SIZE, L"%hs", stackTrace);

	wchar_t header[36]{};
	swprintf_s(header, 36, L"%hs %x", context.ExceptionName, context.ExceptionCode);

	auto& options = Logger::GetInstance()->GetOptions();
	options.Set(LOG_OPTION_NO_PREFIX, true);
	AM_ERR("\n=== UNHANDLED EXCEPTION ===");
	AM_ERRF("%s %x", context.ExceptionName, context.ExceptionCode);
	AM_TRACE(sm_StackTraceBuffer);
	options.Set(LOG_OPTION_NO_PREFIX, false);

#ifdef AM_STANDALONE
	// We close application if any exception happened
	ConstWString footer = L"Application will be closed.";
#else
	// We unload RageAm dll if handled exception occurred (SafeExecute) which doesn't crash game,
	// but if exception occurred somewhere else we can't really do anything about it.
	ConstWString footer = isHandled ? L"GTAV will continue execution but rageAm will be stopped." : L"Application will be closed";
#endif

	// Same as in ::Assert
	TASKDIALOG_BUTTON buttons[3];
	buttons[0].nButtonID = 0;
	buttons[0].pszButtonText = L"Break Debugger / Exit";

	ShowTaskDialog(L"An unhandled exception occurred in RageAm.",
		footer, header, L"Make sure debugger is attached.", sm_StackTraceBuffer, DIALOG_ERROR, buttons, 1);
}
