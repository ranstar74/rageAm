//
// File: logger.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <fstream>
#include <mutex>

#include "types.h"
#include "am/file/path.h"
#include "helpers/flagset.h"
#include "helpers/resharper.h"
#include "helpers/win32.h"

enum eLogLevel
{
	LOG_TRACE,
	LOG_DEBUG,
	LOG_WARNING,
	LOG_ERROR,
};

enum eLogOptions : u32
{
	LOG_OPTION_NONE = 0,

	LOG_OPTION_FILE_ONLY = 1 << 0, // Without additionally printing to console
	LOG_OPTION_NO_PREFIX = 1 << 1,
	LOG_OPTION_SAME_LINE = 1 << 2, // Without '\n' after message
};

// #define AM_ENABLE_FILE_LOG

namespace rageam
{

	class Logger
	{
		// Each launch we create new log folder so we have to maintain how much folders there can be
		static constexpr u32 MAX_LOG_FOLDERS = 8;

		static file::WPath sm_LogDirectory; // Inside data/logs with formatted creation time

		// With stack we can set current logger and direct every new logs in it,
		// for example - fiDevice logs will be redirected in new logger and it will
		// make it easier to locate faulty code.

		static constexpr u32 STACK_SIZE = 64;
		static inline thread_local u32 sm_StackSize = 0;
		static thread_local Logger* sm_Stack[];

		// Keep those in sync with ::eLogLevel

		static constexpr const char* sm_LevelNames[] =
		{
			"Trace", "Debug", "Warning", "Error"
		};
		static constexpr WORD sm_LevelColors[] = // Only for console
		{
			FG_LIGHTGRAY, FG_GRAY, FG_YELLOW, FG_LIGHTRED
		};

		static void FindAndRemoveOldLogFolders();
		// Makes sure that log directory is created, sets console locale
		static void EnsureInitialized();
		// Puts default logger in current thread storage
		static void EnsureThreadInitialized(Logger* defaultLogger);

#ifdef AM_ENABLE_FILE_LOG
		std::wofstream m_Stream;
#endif
		ConstString m_Name;
		FlagSet<eLogOptions> m_Options;
#ifndef AM_STANDALONE
		FILE* m_ConsoleFile = nullptr;
#endif
	public:
		Logger(ConstString name, FlagSet<eLogOptions> options = LOG_OPTION_NONE);
		~Logger();

		void Log(eLogLevel level, const wchar_t* msg);
		void Log(eLogLevel level, const char* msg);

		PRINTF_ATTR(3, 4) void LogFormat(eLogLevel level, ConstString fmt, ...);
		WPRINTF_ATTR(3, 4) void LogFormat(eLogLevel level, ConstWString fmt, ...);

		FlagSet<eLogOptions>& GetOptions() { return m_Options; }

		/**
		 * \brief Gets relative path to directory where all logs are written to.
		 */
		static const file::WPath& GetLogsDirectory();

		/**
		 * \brief Pushes given logger on top of the stack and redirects all further logs via
		 * GetInstance and AM_### macros to it.
		 */
		static void Push(Logger* logger);

		/**
		 * \brief Removes logger from top of the stack, redirecting all further logs to previous logger.
		 */
		static void Pop();

		/**
		 * \brief Gets logger instance from top of the stack. By default it's the 'general' one.
		 */
		static Logger* GetInstance();
	};

	class LoggerScoped
	{
		Logger* m_Logger;
	public:
		LoggerScoped(Logger& logger) : m_Logger(&logger)
		{
			Logger::Push(m_Logger);
		}

		~LoggerScoped()
		{
			Logger::Pop();
		}
	};
}

inline rageam::Logger* GetLogger() { return rageam::Logger::GetInstance(); }

#define AM_TRACE(msg)			rageam::Logger::GetInstance()->Log(LOG_TRACE, msg)
#define AM_WARNING(msg)			rageam::Logger::GetInstance()->Log(LOG_WARNING, msg)
#define AM_ERR(msg)				rageam::Logger::GetInstance()->Log(LOG_ERROR, msg)

#define AM_TRACEF(fmt, ...)		rageam::Logger::GetInstance()->LogFormat(LOG_TRACE, fmt, __VA_ARGS__)
#define AM_WARNINGF(fmt, ...)	rageam::Logger::GetInstance()->LogFormat(LOG_WARNING, fmt, __VA_ARGS__)
#define AM_ERRF(fmt, ...)		rageam::Logger::GetInstance()->LogFormat(LOG_ERROR, fmt, __VA_ARGS__)

#ifdef _DEBUG
#define AM_DEBUG(msg)			rageam::Logger::GetInstance()->Log(LOG_DEBUG, msg)
#define AM_DEBUGF(fmt, ...)		rageam::Logger::GetInstance()->LogFormat(LOG_DEBUG, fmt, __VA_ARGS__)
#else
#define AM_DEBUG(msg)
#define AM_DEBUGF(fmt, ...)
#endif
