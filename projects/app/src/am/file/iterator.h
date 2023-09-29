//
// File: iterator.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>

#include "am/time/datetime.h"
#include "am/system/asserts.h"
#include "am/file/path.h"
#include <helpers/win32.h>

namespace rageam::file
{
	struct FindData
	{
		WPath				Path;
		u64					Size;
		u32					Attributes;
		DateTime			LastWriteTime;
	};

	class Iterator
	{
		ConstWString		m_SearchPattern;
		WPath				m_SearchDirectory;
		bool				m_Started = false;
		WIN32_FIND_DATA		m_FindData = {};
		HANDLE				m_SearchHandle = INVALID_HANDLE_VALUE;

		bool IterateNext()
		{
			if (!m_Started)
			{
				m_SearchHandle = FindFirstFileW(m_SearchPattern, &m_FindData);
				m_Started = true;
				return m_SearchHandle != INVALID_HANDLE_VALUE;
			}
			return FindNextFileW(m_SearchHandle, &m_FindData);
		}
	public:
		Iterator(ConstWString searchPattern)
		{
			m_SearchPattern = searchPattern;

			// Get rid of mask part in "dir/*.*"
			m_SearchDirectory = WPath(searchPattern).GetParentDirectory();
		}

		~Iterator()
		{
			Reset();
		}

		void Reset()
		{
			if(m_SearchHandle != INVALID_HANDLE_VALUE)
				FindClose(m_SearchHandle);
			m_SearchHandle = INVALID_HANDLE_VALUE;
			m_Started = false;
		}

		bool Next()
		{
			if (!IterateNext())
				return false;

			// Those two are file system reserved folders
			if (String::Equals(m_FindData.cFileName, L".") ||
				String::Equals(m_FindData.cFileName, L".."))
				return Next();

			return true;
		}

		void GetCurrent(FindData& data) const
		{
			data.Path = m_SearchDirectory / m_FindData.cFileName;
			data.Attributes = m_FindData.dwFileAttributes;
			data.Size = TODWORD64(m_FindData.nFileSizeLow, m_FindData.nFileSizeHigh);
			data.LastWriteTime = DateTime(m_FindData.ftLastWriteTime);
		}
	};
}
