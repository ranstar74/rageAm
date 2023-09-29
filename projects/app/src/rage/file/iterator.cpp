#include "iterator.h"

#include <Windows.h>
#include <cstdio>
#include <shlwapi.h>

#include "am/string/char.h"

bool rage::fiIterator::Begin()
{
	m_Initialized = true;
	m_Device = fiDevice::GetDeviceImpl(m_Path);
	if (!m_Device)
		return false;

	m_FindHandle = m_Device->FindFileBegin(m_Path, m_FindData);
	return m_FindHandle != FI_INVALID_HANDLE;
}

bool rage::fiIterator::FindNext()
{
	bool hasAny = false;
	if (!m_Initialized)
	{
		hasAny = Begin();
	}
	else if (m_FindHandle != FI_INVALID_HANDLE)
	{
		hasAny = m_Device->FindFileNext(m_FindHandle, m_FindData);
	}

	// Compose full path to current file
	if (hasAny)
	{
		ImmutableString path = m_Path.GetCStr();
		// If path is not a file
		if (path.IndexOf('.') == -1)
			m_FilePath = m_Path / m_FindData.FileName;
		else
			m_FilePath = m_Path;
	}

	return hasAny;
}

rage::fiIterator::fiIterator(ConstString path, ConstString searchPattern)
{
	m_Path = path;
	String::Copy(m_SearchPattern, FI_MAX_NAME, searchPattern);
}

rage::fiIterator::~fiIterator()
{
	if (m_Device && m_FindHandle != FI_INVALID_HANDLE)
		m_Device->FindFileEnd(m_FindHandle);
}

bool rage::fiIterator::Next()
{
	// fiDevice doesn't support actual wildcard search, we'll add it here
	while (true)
	{
		if (!FindNext())
			return false;

		// Those two directories are reserved in windows by file system
		if (String::Equals(m_FindData.FileName, ".") || String::Equals(m_FindData.FileName, ".."))
			continue;

		// Filter out system files that can't be even accessed (such as page file)
		if (m_Device->GetAttributes(m_FilePath) == FI_INVALID_ATTRIBUTES)
			continue;

		// fiDevice search doesn't support masks (though it looks like initially they planned to support it)
		// so just use the same mask comparison function that WinApi search uses
		if (PathMatchSpecA(m_FindData.FileName, m_SearchPattern))
			return true;
	}
}

bool rage::fiIterator::IsDirectory() const
{
	return m_FindData.FileAttributes & FI_ATTRIBUTE_DIRECTORY;
}
