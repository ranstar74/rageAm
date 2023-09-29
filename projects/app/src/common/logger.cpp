#include "logger.h"

#include <cassert>
#include <mutex>
#include <xiosbase>
#include <io.h>
#include <fcntl.h>
#include <filesystem>

#include "am/file/iterator.h"
#include "am/string/string.h"
#include "am/string/stringwrapper.h"
#include "am/system/asserts.h"
#include "am/system/datamgr.h"
#include "am/system/ptr.h"
#include "am/time/datetime.h"
#include "helpers/flagset.h"
#include "helpers/win32.h"
#include "rage/atl/array.h"

#include "am/file/path.h"
#include "rage/atl/fixedarray.h"

rageam::file::WPath rageam::Logger::sm_LogDirectory;
thread_local rageam::Logger* rageam::Logger::sm_Stack[STACK_SIZE]{};

// To prevent initialization fiasco
static std::recursive_mutex& GetMutex()
{
	static std::recursive_mutex mutex;
	return mutex;
}

void rageam::Logger::FindAndRemoveOldLogFolders()
{
	return;
	// Pseudo Code:
	// LogsDirectory
	//		.GetFolders()
	//		.OrderByDescending(x => x.LastWriteTime)
	//		.Skip(MAX_LOG_FOLDERS)
	//		.ForEach(x => Directory.Delete(x.Path))

	struct LogFolder
	{
		file::WPath Path;
		DateTime LastWriteTime;
	};

	// We can't use dynamic allocations here because logger can be called from allocator
	// 16 folders is really more than enough because if user doesn't purposefully
	// duplicate log folders, we wouldn't need more than one
	rage::atFixedArray<LogFolder, MAX_LOG_FOLDERS + 8, u32> logFolders;

	// Parse all log folders and their last edit time
	const file::WPath& logsDir = DataManager::GetLogsFolder();
	file::Iterator it(logsDir / L"*");
	file::FindData entry;
	while (it.Next())
	{
		it.GetCurrent(entry);

		logFolders.Construct(entry.Path, entry.LastWriteTime);

		// Not enough space, will clean up next time LMAO
		// TODO: This whole function is bullshit
		if (logFolders.GetSize() == logFolders.GetCapacity())
			break;
	}

	if (logFolders.GetSize() < MAX_LOG_FOLDERS)
		return;

	// Sort by edit time so we can find the oldest folders
	logFolders.Sort([](const LogFolder& lhs, const LogFolder& rhs)
		{
			return lhs.LastWriteTime > rhs.LastWriteTime;
		});

	// Remove oldest folders
	for (u32 i = MAX_LOG_FOLDERS; i < logFolders.GetSize(); i++)
	{
		ConstWString path = logFolders[i].Path;

		// TODO: ... recurse delete
	}
}

void rageam::Logger::EnsureInitialized()
{
	static bool initialized = false;
	if (initialized)
		return;

	// TODO: UTF16 is broken with alloc console
#ifdef AM_STANDALONE
	// Support wide characters in console
	_setmode(_fileno(stdout), _O_U16TEXT);
#endif

#ifdef AM_ENABLE_FILE_LOG

	// Format logs directory file name with current time
	DateTime time = DateTime::Now();
	char timeFormatted[20];
	time.Format(timeFormatted, 20, "yyyy-MM-ddTHH_mm_ss");
	wchar_t folderName[20];
	String::ToWide(folderName, 20, timeFormatted);

	CreateDirectoryW(DataManager::GetLogsFolder(), NULL);

	sm_LogDirectory = DataManager::GetLogsFolder() / folderName;

	// Note that we can't use assert / verify here because logger is not initialized and it will cause stack overflow...

	if (!CreateDirectoryW(sm_LogDirectory, NULL))
	{
		ConstWString errorMsg = String::FormatTemp(
			L"Failed to create logs directory %ls, Last Error: %u", sm_LogDirectory.GetCStr(), GetLastError());
		MessageBoxW(NULL, errorMsg, L"Initialization error", MB_ICONERROR);
		abort();
	}
	CompressDirectory(sm_LogDirectory);

	initialized = true;

	FindAndRemoveOldLogFolders();
#endif
}

void rageam::Logger::EnsureThreadInitialized(Logger* defaultLogger)
{
	static thread_local bool threadInitialized = false;
	if (threadInitialized)
		return;
	threadInitialized = true;

	Push(defaultLogger);
}

rageam::Logger::Logger(ConstString name, FlagSet<eLogOptions> options)
{
	EnsureInitialized();

	m_Name = name;
	m_Options = options;

#ifndef AM_STANDALONE
	if (AllocConsole())
	{
		freopen_s(&m_ConsoleFile, "CONOUT$", "w", stdout);
	}
#endif

#ifdef AM_ENABLE_FILE_LOG
	wchar_t fileName[64];
	String::ToWide(fileName, 64, name);
	file::WPath filePath = sm_LogDirectory / fileName;
	filePath += L".txt";

	m_Stream.open(filePath, std::ios_base::trunc);
	AM_ASSERT(m_Stream.is_open(), L"Logger() -> Failed to create log file %ls", filePath.GetCStr());

	if (!m_Options.IsSet(LOG_OPTION_NO_PREFIX))
		m_Stream << "Level, Time (HH/MM/SS), Message\n";
#endif
}

rageam::Logger::~Logger()
{
#ifdef AM_ENABLE_FILE_LOG
	m_Stream.close();
#endif

#ifndef AM_STANDALONE
	if (m_ConsoleFile)
	{
		fclose(m_ConsoleFile);
		FreeConsole();
		m_ConsoleFile = nullptr;
	}
#endif
}

void rageam::Logger::Log(eLogLevel level, ConstWString msg)
{
	std::unique_lock lock(GetMutex());

	WORD oldColor = SetConsoleColor(sm_LevelColors[level]);

	if (!m_Options.IsSet(LOG_OPTION_NO_PREFIX))
	{
		// LEVEL, HH:mm:ss
		wchar_t prefix[48];

		char timeFormatted[32];
		DateTime now = DateTime::Now();
		now.Format(timeFormatted, 32, "T");

		swprintf_s(prefix, 48, L"%hs, %hs ", sm_LevelNames[level], timeFormatted);

		// Console output have extra prefix with logger name
		if (!m_Options.IsSet(LOG_OPTION_FILE_ONLY))
		{
			wprintf(L"[%hs] %ls", m_Name, prefix);
		}
#ifdef AM_ENABLE_FILE_LOG
		m_Stream << prefix;
#endif
	}

	SetConsoleColor(oldColor);

	bool sameLine = m_Options.IsSet(LOG_OPTION_SAME_LINE);

	if (!m_Options.IsSet(LOG_OPTION_FILE_ONLY))
	{
		wprintf(L"%s", msg);
		if (!sameLine)
			wprintf(L"\n");
	}

#ifdef AM_ENABLE_FILE_LOG
	m_Stream << msg;
	if (!sameLine)
		m_Stream << L"\n";
#endif
	}

void rageam::Logger::Log(eLogLevel level, const char* msg)
{
	std::unique_lock lock(GetMutex());

	static wchar_t buffer[2048];
	String::ToWide(buffer, 2048, msg);
	Log(level, buffer);
}

void rageam::Logger::LogFormat(eLogLevel level, ConstString fmt, ...)
{
	std::unique_lock lock(GetMutex());

	static char buffer[2048];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buffer, 2048, fmt, args);
	va_end(args);

	LogFormat(level, L"%hs", buffer);
}

void rageam::Logger::LogFormat(eLogLevel level, ConstWString fmt, ...)
{
	std::unique_lock lock(GetMutex());

	static wchar_t buffer[2048];
	va_list args;
	va_start(args, fmt);
	vswprintf_s(buffer, 2048, fmt, args);
	va_end(args);

	Log(level, buffer);
}

const rageam::file::WPath& rageam::Logger::GetLogsDirectory()
{
	std::unique_lock lock(GetMutex());

	EnsureInitialized();
	return sm_LogDirectory;
}

void rageam::Logger::Push(Logger* logger)
{
	std::unique_lock lock(GetMutex());

	AM_ASSERT(sm_StackSize + 1 != STACK_SIZE, "Logger::Push() -> Stack is corrupted.");
	sm_Stack[sm_StackSize++] = logger;
}

void rageam::Logger::Pop()
{
	std::unique_lock lock(GetMutex());

	AM_ASSERT(sm_StackSize != 0, "Logger::Pop() -> Stack is corrupted.");
	sm_StackSize--;
}

rageam::Logger* rageam::Logger::GetInstance()
{
	std::unique_lock lock(GetMutex());

	EnsureInitialized();
	static Logger general("general");
	EnsureThreadInitialized(&general);

	return sm_Stack[sm_StackSize - 1]; // Last element in stack
}
