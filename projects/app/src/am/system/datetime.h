//
// File: datetime.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>

#include "common/types.h"

namespace rageam
{
	enum eMonth
	{
		January = 1,
		February = 2,
		March = 3,
		April = 4,
		May = 5,
		June = 6,
		July = 7,
		August = 8,
		September = 9,
		October = 10,
		November = 11,
		December = 12
	};

	using W32FileTime = FILETIME;
	using W32SystemTime = SYSTEMTIME;

	bool IsLeapYear(int year);

	int DaysInYear(int year);
	int DaysInMonth(int year, int month);

	/**
	 * \brief Utility to work with 'FileTime'.
	 * \n A file time is a 64-bit value that represents the number of
	 * 100-nanosecond intervals that have elapsed since
	 * 12:00 A.M. January 1, 1601 Coordinated Universal Time (UTC).
	 * \remarks More info on https://learn.microsoft.com/en-us/windows/win32/sysinfo/file-times
	 */
	class DateTime
	{
		static constexpr u64 NANOSECONDS_IN_SECOND = 1000ull * 1000ull * 1000ull;
		static constexpr u64 TICKS_IN_SECOND = NANOSECONDS_IN_SECOND / 100;
		static constexpr u64 TICKS_IN_MINUTE = TICKS_IN_SECOND * 60;
		static constexpr u64 TICKS_IN_HOUR = TICKS_IN_MINUTE * 60;
		static constexpr u64 TICKS_IN_DAY = TICKS_IN_HOUR * 24;
		static constexpr u64 TICKS_IN_WEEK = TICKS_IN_DAY * 7;
		static constexpr u64 TICKS_IN_MILLISECOND = TICKS_IN_SECOND / 1000;
		static constexpr u64 TICKS_IN_MICROSECOND = TICKS_IN_MILLISECOND / 1000;
		static constexpr u64 TICKS_IN_NANOECOND = TICKS_IN_MICROSECOND / 1000;

		// Converts WinApi FILETIME struct to unsigned 64 bit integer.
		u64 WinFileTimeToU64(W32FileTime winFileTime) const;
		// Converts unsigned 64 bit integer to WinApi FILETIME.
		W32FileTime U64ToWinFileTime(u64 ticks) const;

		// Recomputes actual time represented by ticks (m_TimeCache). Must be called if m_Ticks changes.
		void ComputeTimeCache();

		// Sets given ticks and recomputes time cache.
		void SetTicks(u64 ticks);

		// We additionally store actual time (year, month, day) to minimize amount of requests to WinApi.
		W32SystemTime m_TimeCache = {};
		u64 m_Ticks = 0;
	public:
		DateTime() = default;
		DateTime(u64 ticks);
		DateTime(W32FileTime winFileTime);

		DateTime(int year, int month, int day);
		DateTime(int year, int month, int day, int hour, int minute);
		DateTime(int year, int month, int day, int hour, int minute, int second);
		DateTime(int year, int month, int day, int hour, int minute, int second, int milliseconds);

		/**
		 * \brief Returns current local (system) time.
		 */
		static DateTime Now();

		/**
		 * \brief Returns current UTC time.
		 */
		static DateTime UtcNow();

		/**
		 * \brief Converts current time from Local to UTC.
		 */
		DateTime ToUniversalTime() const;

		/**
		 * \brief Converts current time from UTC to Local.
		 */
		DateTime ToLocalTime() const;

		/**
		 * \brief Gets ticks that represent number of 100-nanosecond intervals since January 1, 1601 00:00:00.
		 */
		u64 GetTicks() const { return m_Ticks; }

		/**
		 * \brief Converts current time to WinApi FILETIME.
		 */
		W32FileTime ConvertToFileTime() const;

		/**
		 * \brief Gets today's date with time set to 00:00:00.
		 */
		DateTime Today() const;

		/**
		 * \brief Formats this date into string.
		 * \remarks Supported specifiers:
		 * \n yyyy, M, MM, d, dd, H, HH, m, mm, s, ss.
		 * \n Standard formats:
		 * \n d - Short date pattern, 15/6/2009
		 * \n g - General date/time pattern (short time), 15/06/2009 13:45
		 * \n G - General date/time pattern (long time), 15/06/2009 13:45:30
		 * \n s - ISO 8601, 2009-06-15T13:45:30
		 * \n t - Short time pattern, 13:45
		 * \n T - Long time pattern, 13:45:30
		 * \n Any other character is copied unchanged.
		 *
		 * \n This fully matches .NET DateTime Format, see:
		 * \n https://learn.microsoft.com/en-us/dotnet/standard/base-types/custom-date-and-time-format-strings
		 */
		void Format(char* buffer, int bufferSize, const char* fmt) const;

		/**
		 * \brief Formats time passed.
		 * \n "Now" 0 - 1 minute passed
		 * \n "# minutes ago" 1 - 60 minutes passed
		 * \n "Hour ago" 60 - 120 minutes passed
		 * \n "Today, 15:30"
		 * \n "Yesterday, 15:30"
		 * \n "5 days ago" 2 - 7 days passed
		 * \n "30/05/2023 15:30" More than 7 days passed
		 */
		void FormatTimeSince(char* buffer, int bufferSize) const;

		DateTime Subtract(const DateTime& other)	const;
		DateTime AddYears(int value)				const;
		DateTime AddMonths(int value)				const;
		DateTime AddWeeks(int value)				const { return DateTime(m_Ticks + (u64)value * TICKS_IN_WEEK); }
		DateTime AddDays(int value)					const { return DateTime(m_Ticks + (u64)value * TICKS_IN_DAY); }
		DateTime AddHours(int value)				const { return DateTime(m_Ticks + (u64)value * TICKS_IN_HOUR); }
		DateTime AddMinutes(int value)				const { return DateTime(m_Ticks + (u64)value * TICKS_IN_MINUTE); }
		DateTime AddSeconds(int value)				const { return DateTime(m_Ticks + (u64)value * TICKS_IN_SECOND); }
		DateTime AddMicroseconds(int value)			const { return DateTime(m_Ticks + (u64)value * TICKS_IN_MICROSECOND); }
		DateTime AddMilliseconds(int value)			const { return DateTime(m_Ticks + (u64)value * TICKS_IN_MILLISECOND); }
		DateTime AddTicks(u64 value)				const { return DateTime(m_Ticks + value); }

		bool IsLeapYear()							const { return rageam::IsLeapYear(Year()); }

		int Day()									const { return m_TimeCache.wDay; }
		int DayOfWeek()								const { return m_TimeCache.wDayOfWeek; }
		int Hour()									const { return m_TimeCache.wHour; }
		int Millisecond()							const { return m_TimeCache.wMilliseconds; }
		int Minute()								const { return m_TimeCache.wMinute; }
		int Month()									const { return m_TimeCache.wMonth; }
		int Second()								const { return m_TimeCache.wSecond; }
		int Year()									const { return m_TimeCache.wYear; }

		double TotalMicroseconds()					const { return (double)m_Ticks / TICKS_IN_MICROSECOND; }
		double TotalMilliseconds()					const { return (double)m_Ticks / TICKS_IN_MILLISECOND; }
		double TotalSeconds()						const { return (double)m_Ticks / TICKS_IN_SECOND; }
		double TotalMinutes()						const { return (double)m_Ticks / TICKS_IN_MINUTE; }
		double TotalHours()							const { return (double)m_Ticks / TICKS_IN_HOUR; }
		double TotalDays()							const { return (double)m_Ticks / TICKS_IN_DAY; }
		double TotalWeeks()							const { return (double)m_Ticks / TICKS_IN_WEEK; }

		bool operator==(const DateTime& other)		const { return m_Ticks == other.m_Ticks; }
		bool operator>(const DateTime& other)		const { return m_Ticks > other.m_Ticks; }
		bool operator<(const DateTime& other)		const { return m_Ticks < other.m_Ticks; }
		bool operator>=(const DateTime& other)		const { return m_Ticks >= other.m_Ticks; }
		bool operator<=(const DateTime& other)		const { return m_Ticks <= other.m_Ticks; }

		DateTime& operator=(const DateTime& other) = default;
		DateTime operator-(const DateTime& other) const;

		operator u64() const { return GetTicks(); }
	};
}
