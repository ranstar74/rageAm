#include "datetime.h"

#include "am/system/asserts.h"

#include <cmath>
#include <stdio.h>

bool rageam::IsLeapYear(int year)
{
	if (year % 4 != 0) return false;
	if (year % 400 == 0) return true;
	if (year % 100 == 0) return false;
	return true;
}

int rageam::DaysInYear(int year)
{
	return IsLeapYear(year) ? 366 : 365;
}

int rageam::DaysInMonth(int year, int month)
{
	AM_ASSERT(month != 0 && month < 12, "DaysInMonth() -> Month %i is out of range.", month);
	static constexpr int s_DaysInMonth[] =
	{
		0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};

	if (month == February && IsLeapYear(year))
		return 29;
	return s_DaysInMonth[month];
}

u64 rageam::DateTime::WinFileTimeToU64(W32FileTime winFileTime) const
{
	return (u64)winFileTime.dwLowDateTime | (u64)winFileTime.dwHighDateTime << 32;
}

rageam::W32FileTime rageam::DateTime::U64ToWinFileTime(u64 ticks) const
{
	W32FileTime winFileTime;
	winFileTime.dwLowDateTime = ticks & 0xFFFFFFFF;
	winFileTime.dwHighDateTime = ticks >> 32 & 0xFFFFFFFF;
	return winFileTime;
}

void rageam::DateTime::ComputeTimeCache()
{
	W32FileTime winFileTime = U64ToWinFileTime(m_Ticks);
	FileTimeToSystemTime(&winFileTime, &m_TimeCache);
}

void rageam::DateTime::SetTicks(u64 ticks)
{
	if (m_Ticks == ticks)
		return;
	m_Ticks = ticks;
	ComputeTimeCache();
}

rageam::DateTime::DateTime(u64 ticks)
{
	SetTicks(ticks);
}

rageam::DateTime::DateTime(W32FileTime winFileTime)
{
	SetTicks(WinFileTimeToU64(winFileTime));
}

rageam::DateTime::DateTime(int year, int month, int day)
	: DateTime(year, month, day, 0, 0, 0, 0) {}

rageam::DateTime::DateTime(int year, int month, int day, int hour, int minute)
	: DateTime(year, month, day, hour, minute, 0, 0) {}

rageam::DateTime::DateTime(int year, int month, int day, int hour, int minute, int second)
	: DateTime(year, month, day, hour, minute, second, 0) {}

rageam::DateTime::DateTime(int year, int month, int day, int hour, int minute, int second, int milliseconds)
{
	W32SystemTime winSystemTime = {};
	winSystemTime.wYear = (u16)year;
	winSystemTime.wMonth = (u16)month;
	winSystemTime.wDay = (u16)day;
	winSystemTime.wHour = (u16)hour;
	winSystemTime.wMinute = (u16)minute;
	winSystemTime.wSecond = (u16)second;
	winSystemTime.wMilliseconds = (u16)milliseconds;

	W32FileTime winFileTime;
	SystemTimeToFileTime(&winSystemTime, &winFileTime);
	SetTicks(WinFileTimeToU64(winFileTime));
}

rageam::DateTime rageam::DateTime::Now()
{
	W32FileTime winUtcFileTime;
	W32FileTime winLocalFileTime;
	GetSystemTimeAsFileTime(&winUtcFileTime);
	FileTimeToLocalFileTime(&winUtcFileTime, &winLocalFileTime);
	return DateTime(winLocalFileTime);
}

rageam::DateTime rageam::DateTime::UtcNow()
{
	W32FileTime winFileTime;
	GetSystemTimeAsFileTime(&winFileTime);
	return DateTime(winFileTime);
}

rageam::DateTime rageam::DateTime::ToUniversalTime() const
{
	W32FileTime winUtcFileTime;
	W32FileTime winLocalFileTime = ConvertToFileTime();
	LocalFileTimeToFileTime(&winLocalFileTime, &winUtcFileTime);
	return DateTime(winUtcFileTime);
}

rageam::DateTime rageam::DateTime::ToLocalTime() const
{
	W32FileTime winLocalFileTime;
	W32FileTime winUtcFileTime = ConvertToFileTime();
	FileTimeToLocalFileTime(&winUtcFileTime, &winLocalFileTime);
	return DateTime(winLocalFileTime);
}

rageam::W32FileTime rageam::DateTime::ConvertToFileTime() const
{
	return U64ToWinFileTime(m_Ticks);
}

rageam::DateTime rageam::DateTime::Today() const
{
	u64 ticks = m_Ticks;
	ticks -= m_TimeCache.wMilliseconds * TICKS_IN_MILLISECOND;
	ticks -= m_TimeCache.wSecond * TICKS_IN_SECOND;
	ticks -= m_TimeCache.wMinute * TICKS_IN_MINUTE;
	ticks -= m_TimeCache.wHour * TICKS_IN_HOUR;
	return DateTime(ticks);
}

void rageam::DateTime::Format(char* buffer, int bufferSize, const char* fmt) const
{
	char* bCursor = buffer;		// Emitting position in buffer
	const char* fCursor = fmt;	// Position in format string

	int bAvail = bufferSize;
	int fAvail = (int)strlen(fmt);

	// Standard format
	if (fAvail == 1)
	{
		switch (fmt[0])
		{
		case 'd':
			// "d" - Short date pattern, 15/6/2009
			sprintf_s(buffer, bufferSize, "%d/%d/%d", Day(), Month(), Year());
			return;
		case 'g':
			// "g" - General date / time pattern(short time), 15/06/2009 13:45
			sprintf_s(buffer, bufferSize, "%02d/%02d/%04d %02d:%02d", Day(), Month(), Year(), Hour(), Minute());
			return;
		case 'G':
			// "g" - General date / time pattern(long time), 15/06/2009 13:45:15
			sprintf_s(buffer, bufferSize, "%02d/%02d/%04d %02d:%02d:%02d", Day(), Month(), Year(), Hour(), Minute(), Second());
			return;
		case 's':
			// "s" - ISO 8601, 2009-06-15T13:45:30
			sprintf_s(buffer, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02d", Year(), Month(), Day(), Hour(), Minute(), Second());
			return;
		case 't':
			// "t" - Short time pattern, 13:45
			sprintf_s(buffer, bufferSize, "%02d:%02d", Hour(), Minute());
			return;
		case 'T':
			// T - Long time pattern, 13:45:30
			sprintf_s(buffer, bufferSize, "%02d:%02d:%02d", Hour(), Minute(), Second());
			return;
		}
	}

	// Custom format
	while (fAvail != 0)
	{
		AM_ASSERT(bAvail > 0, "DateTime::Format() -> Out of buffer space!");

		// Note: Longer formats, such as "ss", must come before "s"

		int movSize = 0;

		// "yyyy" - The year as a four-digit number.
		if (fAvail >= 4 && fCursor[0] == 'y' && fCursor[1] == 'y' && fCursor[2] == 'y' && fCursor[3] == 'y')
		{
			sprintf_s(bCursor, bAvail, "%04d", Year());
			movSize = 4;
		}

		// "MM" - The month, from 01 through 12.
		else if (fAvail >= 2 && fCursor[0] == 'M' && fCursor[1] == 'M')
		{
			sprintf_s(bCursor, bAvail, "%02d", Month());
			movSize = 2;
		}

		// "M" - The month, from 1 through 12.
		else if (fAvail >= 1 && fCursor[0] == 'M')
		{
			sprintf_s(bCursor, bAvail, "%d", Month());
			movSize = 1;
		}

		// "dd" - The day of the month, from 01 through 31.
		else if (fAvail >= 2 && fCursor[0] == 'd' && fCursor[1] == 'd')
		{
			sprintf_s(bCursor, bAvail, "%02d", Day());
			movSize = 2;
		}

		// "d" - The day of the month, from 1 through 31.
		else if (fAvail >= 1 && fCursor[0] == 'd')
		{
			bAvail -= sprintf_s(bCursor, bAvail, "%d", Day());
			movSize += 1;
		}

		// "HH" - The hour, using a 24-hour clock from 0 to 23.
		else if (fAvail >= 2 && fCursor[0] == 'H' && fCursor[1] == 'H')
		{
			sprintf_s(bCursor, bAvail, "%02d", Hour());
			movSize = 2;
		}

		// "H" - The hour, using a 24-hour clock from 0 to 23.
		else if (fAvail >= 1 && fCursor[0] == 'H')
		{
			sprintf_s(bCursor, bAvail, "%d", Hour());
			movSize = 1;
		}

		// "mm" - The minute, from 00 through 59.
		else if (fAvail >= 2 && fCursor[0] == 'm' && fCursor[1] == 'm')
		{
			sprintf_s(bCursor, bAvail, "%02d", Minute());
			movSize = 2;
		}

		// "m" - The minute, from 0 through 59.
		else if (fAvail >= 1 && fCursor[0] == 'm')
		{
			bAvail -= sprintf_s(bCursor, bAvail, "%d", Minute());
			movSize += 1;
		}

		// "ss" - The second, from 00 through 59.
		else if (fAvail >= 2 && fCursor[0] == 's' && fCursor[1] == 's')
		{
			sprintf_s(bCursor, bAvail, "%02d", Second());
			movSize = 2;
		}

		// "s" - The second, from 0 through 59.
		else if (fAvail >= 1 && fCursor[0] == 's')
		{
			sprintf_s(buffer, bAvail, "%d", Second());
			movSize = 1;
		}

		// Simply copy any other character
		else
		{
			bCursor[0] = fCursor[0];
			movSize = 1;
		}

		fCursor += movSize;
		bCursor += movSize;
		fAvail -= movSize;
		bAvail -= movSize;
	}

	// Finalize resulting string
	AM_ASSERT(bAvail >= 1, "DateTime::Format() -> Out of buffer space!");
	bCursor[0] = '\0';
}

void rageam::DateTime::FormatTimeSince(char* buffer, int bufferSize) const
{
	DateTime timeSince = Now().Subtract(*this);
	int minutes = static_cast<int>(floor(timeSince.TotalMinutes()));
	int days = static_cast<int>(floor(timeSince.TotalDays()));
	if (minutes == 0)
	{
		sprintf_s(buffer, bufferSize, "%s", "Now");
	}
	else if (minutes < 60)
	{
		sprintf_s(buffer, bufferSize, (minutes == 1 ? "%d minute ago" : "%d minutes ago"), minutes);
	}
	else if (minutes < 120)
	{
		sprintf_s(buffer, bufferSize, "%s", "Hour ago");
	}
	else if (days == 0)
	{
		char timeBuffer[6];
		Format(timeBuffer, 6, "HH:mm");
		sprintf_s(buffer, bufferSize, "Today, %s", timeBuffer);
	}
	else if (days == 1)
	{
		char timeBuffer[6];
		Format(timeBuffer, 6, "HH:mm");
		sprintf_s(buffer, bufferSize, "Yesterday, %s", timeBuffer);
	}
	else if (days <= 6)
	{
		sprintf_s(buffer, bufferSize, "%d days ago, %02d:%02d", days, Hour(), Minute());
	}
	else
	{
		Format(buffer, bufferSize, "g");
	}
}

rageam::DateTime rageam::DateTime::Subtract(const DateTime& other) const
{
	return DateTime(m_Ticks - other.m_Ticks);
}

rageam::DateTime rageam::DateTime::AddYears(int value) const
{
	DateTime result(m_Ticks);
	for (int i = 0; i < value; i++)
	{
		int daysInYear = DaysInYear(result.Year());
		result = result.AddDays(daysInYear);
	}
	return result;
}

rageam::DateTime rageam::DateTime::AddMonths(int value) const
{
	DateTime result(m_Ticks);
	for (int i = 0; i < value; i++)
	{
		int daysInMonth = DaysInMonth(result.Year(), result.Month());
		result = result.AddDays(daysInMonth);
	}
	return result;
}

rageam::DateTime rageam::DateTime::operator-(const DateTime& other) const
{
	return Subtract(other);
}
