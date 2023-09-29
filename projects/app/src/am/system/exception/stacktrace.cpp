#include "stacktrace.h"

#include "common/logger.h"
#include "helpers/compiler.h"
#include "helpers/format.h"
#include "helpers/macro.h"

#include <DbgHelp.h>

rage::sysCriticalSectionToken rageam::StackTracer::sm_Mutex;

bool rageam::SymbolResolver::ResolveSymbol(u64 addr, Info& info)
{
#ifdef AM_STACKTRACE_SYMBOLS
	char symInfoBuffer[sizeof(SYMBOL_INFO) + Info::MAX_SYMBOL_NAME * sizeof(char)]{};
	PSYMBOL_INFO symInfo = (PSYMBOL_INFO)symInfoBuffer;

	symInfo->SizeOfStruct = sizeof SYMBOL_INFO;
	symInfo->MaxNameLen = Info::MAX_SYMBOL_NAME;

	if (!SymFromAddr(sm_Process, addr, NULL, symInfo))
		return false;

	strcpy_s(info.Name, Info::MAX_SYMBOL_NAME, symInfo->Name);

	info.HasName = true;
	info.Offset = static_cast<u32>(addr - symInfo->Address);

	return true;
#else
	return false;
#endif
}

bool rageam::SymbolResolver::ResolveLine(u64 addr, Info& info)
{
#ifdef AM_STACKTRACE_SYMBOLS
	DWORD displacement;

	IMAGEHLP_LINEW64 symLine{};
	symLine.SizeOfStruct = sizeof IMAGEHLP_LINEW64;

	if (!SymGetLineFromAddrW64(sm_Process, addr, &displacement, &symLine))
		return false;

	wcscpy_s(info.FileName, Info::MAX_FILE_NAME, symLine.FileName);

	info.HasFileName = true;
	info.LineNumber = symLine.LineNumber;

	return true;
#else
	return false;
#endif
}

void rageam::SymbolResolver::Load()
{
#ifdef AM_STACKTRACE_SYMBOLS
	if (sm_Loaded)
		return;

	// Don't use GetCurrentProcess() because it returns fake handle and it causes issues when calling from different threads...
	sm_Process = OpenProcess(PROCESS_ALL_ACCESS, TRUE, GetCurrentProcessId());

	SymSetOptions(AM_DEBUG_ONLY(SYMOPT_DEBUG | ) SYMOPT_LOAD_LINES);
	if (!AM_VERIFY(SymInitialize(sm_Process, NULL, TRUE),
		"StackTracer::LoadSymbols() -> Failed to load symbols, last error: %x", GetLastError()))
		return;

	sm_Loaded = true;
#endif
}

void rageam::SymbolResolver::Free()
{
#ifdef AM_STACKTRACE_SYMBOLS
	if (!sm_Loaded)
		return;

	if (!AM_VERIFY(SymCleanup(sm_Process),
		"SymbolResolver::Free() -> Cleanup failed, last error: %x", GetLastError()))
		return;

	CloseHandle(sm_Process);

	sm_Loaded = false;
	sm_Process = NULL;
#endif
}

void rageam::SymbolResolver::Resolve(u64 addr, Info& info)
{
	memset(&info, 0, sizeof Info);

	GetModuleNameFromAddress(addr, info.ModuleName, Info::MAX_MODULE_NAME);

	if (!ResolveSymbol(addr, info))
		return;

	ResolveLine(addr, info);
}

AM_NOINLINE u16 rageam::StackTracer::CaptureCurrent(u32 frameSkip)
{
	u16 frameCount = RtlCaptureStackBackTrace(
		frameSkip + 1 /* This */, STACKTRACE_MAX_FRAMES,
		reinterpret_cast<PVOID*>(&sm_Frames), NULL);

	for (u16 i = 0; i < STACKTRACE_MAX_FRAMES; i++)
	{
		// Because this is return instruction address, every line number will be shifted by one,
		// what we subtract size of 64 bit call instruction (5 bytes)
		sm_Frames[i] -= 5;
	}

	return frameCount;
}

u16 rageam::StackTracer::CaptureFromContext(ExceptionHandler::Context& context)
{
	u16 frameCount = 0;

	STACKFRAME64 stackFrame{};
	stackFrame.AddrPC.Mode = AddrModeFlat;
	stackFrame.AddrPC.Offset = context.ExecutionRecord.Rip;
	stackFrame.AddrStack.Mode = AddrModeFlat;
	stackFrame.AddrStack.Offset = context.ExecutionRecord.Rsp;
	stackFrame.AddrFrame.Mode = AddrModeFlat;
	stackFrame.AddrFrame.Offset = context.ExecutionRecord.Rbp;

	// First address is exact address where exception occurred, there's no need
	// to fix it up like with 'return address' case below
	sm_Frames[frameCount++] = context.ExceptionAddress;

	// Requires symbols to be loaded and CONTEXT copy (which is done by ExceptionHandler::Context)
	while (StackWalk64(IMAGE_FILE_MACHINE_AMD64, GetCurrentProcess(), GetCurrentThread(),
		&stackFrame, &context.ExecutionRecord, NULL,
		SymFunctionTableAccess64, SymGetModuleBase64, NULL))
	{
		u64 addr = stackFrame.AddrReturn.Offset;
		if (!addr)
			break;

		// (copy-paste from ::CaptureCurrent)
		// Because this is return instruction address, every line number will be shifted by one,
		// what we subtract size of 64 bit call instruction (5 bytes)
		addr -= 5;

		sm_Frames[frameCount++] = addr;

		if (frameCount == STACKTRACE_MAX_FRAMES)
			break;
	}

	return frameCount;
}

void rageam::StackTracer::AppendToBuffer(ConstString fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	u32 written = vsprintf_s(sm_Buffer, sm_BufferSize, fmt, args);
	va_end(args);

	sm_Buffer += written;
	sm_BufferSize -= written;
}

void rageam::StackTracer::WriteTo(u16 frameCount, char* buffer, u32 bufferSize)
{
	SymbolResolver::Info info{};

	sm_Buffer = buffer;
	sm_BufferSize = bufferSize;

	AppendToBuffer("Stack Trace: \n");

	for (u16 i = 0; i < frameCount; i++) // .NET trace style
	{
		u64 addr = sm_Frames[i];

		SymbolResolver::Resolve(addr, info);

		// at Module!Address
		if (!info.HasName)
		{
			AppendToBuffer("at %s!0x%p\n", info.ModuleName, reinterpret_cast<pVoid>(addr));
			continue;
		}

		// at Module!Procedure N byte(s)
		if (!info.HasFileName)
		{
			AppendToBuffer("at %s!%s + %u byte(s)\n", info.ModuleName, info.Name, info.Offset);
			continue;
		}

		// at Module!Procedure in File:Line
		AppendToBuffer("at %s!%s in %ls:line %u\n", info.ModuleName, info.Name, info.FileName, info.LineNumber);
	}
}
