#include "local.h"

#include <Windows.h>
#include "common/logger.h"
#include "helpers/win32.h"

void rage::fiDeviceLocal::Win32FindDataToFi(const WIN32_FIND_DATAW& from, fiFindData& to)
{
	String::WideToUtf8(to.FileName, FI_MAX_PATH, from.cFileName);

	to.FileSize = TODWORD64(from.nFileSizeLow, from.nFileSizeHigh);
	to.FileAttributes = from.dwFileAttributes;
	to.LastWriteTime = TODWORD64(from.ftLastWriteTime.dwLowDateTime, from.ftLastWriteTime.dwHighDateTime);
}

ConstWString rage::fiDeviceLocal::ConvertPathToWide(ConstString path)
{
	thread_local wchar_t buffer[MAX_PATH];
	String::Utf8ToWide(buffer, MAX_PATH, path);
	return buffer;
}

rage::fiDeviceLocal* rage::fiDeviceLocal::GetInstance()
{
	static fiDeviceLocal local;
	return &local;
}

fiHandle_t rage::fiDeviceLocal::Open(ConstString path, bool isReadOnly)
{
	DWORD access = GENERIC_READ;
	if (!isReadOnly)
		access |= GENERIC_WRITE;

	ConstWString wPath = ConvertPathToWide(path);

	HANDLE handle = CreateFileW(wPath, access,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		AM_ERRF(L"fiDeviceLocal::Open(%ls, isReadOnly: %u) -> Unable to open file, last error: %#x",
			wPath, isReadOnly, GetLastError());
		return handle;
	}
	return handle;
}

fiHandle_t rage::fiDeviceLocal::OpenBulk(ConstString path, u64& offset)
{
	offset = 0;

	ConstWString wPath = ConvertPathToWide(path);

	HANDLE handle = CreateFileW(wPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		AM_ERRF(L"fiDeviceLocal::OpenBulk(%ls) -> Unable to open file, last error: %#x", wPath, GetLastError());
		return handle;
	}
	return handle;
}

fiHandle_t rage::fiDeviceLocal::CreateBulk(ConstString path)
{
	ConstWString wPath = ConvertPathToWide(path);

	HANDLE handle = CreateFileW(wPath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		AM_ERRF(L"fiDeviceLocal::CreateBulk(%ls) -> Unable to create file, last error: %#x", wPath, GetLastError());
		return handle;
	}
	return handle;
}

fiHandle_t rage::fiDeviceLocal::Create(ConstString path)
{
	ConstWString wPath = ConvertPathToWide(path);

	HANDLE handle = CreateFileW(wPath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (handle == INVALID_HANDLE_VALUE)
	{
		AM_ERRF(L"fiDeviceLocal::Create(%ls) -> Unable to create file, last error: %#x", wPath, GetLastError());
		return handle;
	}
	return handle;
}

u32 rage::fiDeviceLocal::Read(fiHandle_t file, pVoid buffer, u32 size)
{
	DWORD bytesRead;
	if (!ReadFile(file, buffer, size, &bytesRead, NULL))
	{
		AM_ERRF("fiDeviceLocal::Read(file: %p) -> Unable to read file, last error: %#x", file, GetLastError());
		return -1;
	}
	return bytesRead;
}

u32 rage::fiDeviceLocal::ReadBulk(fiHandle_t file, u64 offset, pVoid buffer, u32 size)
{
	OVERLAPPED overlapped{};
	SET_OVERLAPPED_OFFSET(overlapped, offset);

	char* cBuffer = static_cast<char*>(buffer);

	DWORD bytesRead;
	if (!ReadFile(file, cBuffer, size, &bytesRead, &overlapped))
	{
		AM_ERRF("fiDeviceLocal::ReadBulk(file: %p) -> Unable to read file, last error: %#x", file, GetLastError());
		return FI_INVALID_RESULT;
	}
	return bytesRead;
}

u32 rage::fiDeviceLocal::WriteOverlapped(fiHandle_t file, u64 offset, pConstVoid buffer, u32 size)
{
	OVERLAPPED overlapped{};
	SET_OVERLAPPED_OFFSET(overlapped, offset);

	DWORD bytesWritten;
	if (!WriteFile(file, buffer, size, &bytesWritten, &overlapped))
	{
		AM_ERRF("fiDeviceLocal::WriteOverlapped(file: %p) -> Unable to write file, last error: %#x", file, GetLastError());
		return FI_INVALID_RESULT;
	}
	return bytesWritten;
}

u32 rage::fiDeviceLocal::Write(fiHandle_t file, pConstVoid buffer, u32 size)
{
	DWORD bytesWritten;

	if (!WriteFile(file, buffer, size, &bytesWritten, NULL))
	{
		AM_ERRF("fiDeviceLocal::Write(file: %p) -> Unable to write file, last error: %#x", file, GetLastError());
		return FI_INVALID_RESULT;
	}
	return bytesWritten;
}

u64 rage::fiDeviceLocal::Seek64(fiHandle_t file, s64 offset, eFiSeekWhence whence)
{
	LARGE_INTEGER lNewOffset;

	if (!SetFilePointerEx(file, ToLargeInteger(offset), &lNewOffset, whence))
	{
		AM_ERRF("fiDeviceLocal::Seek64(file: %p, offset: %lld) -> Unable to set position, last error: %#x",
			file, offset, GetLastError());
		return FI_INVALID_RESULT;
	}
	return lNewOffset.QuadPart;
}

bool rage::fiDeviceLocal::Close(fiHandle_t file)
{
	BOOL result = CloseHandle(file);
	if (!result)
		AM_ERRF("fiDeviceLocal::Close(%p) -> Failed to close file, last error: %#x", file, GetLastError());
	return result;
}

bool rage::fiDeviceLocal::Delete(ConstString path)
{
	ConstWString wPath = ConvertPathToWide(path);

	BOOL result = DeleteFileW(wPath);
	if (!result)
		AM_ERRF(L"fiDeviceLocal::Delete(%ls) -> Failed to delete file, last error: %#x", wPath, GetLastError());
	return result;
}

bool rage::fiDeviceLocal::Rename(ConstString oldPath, ConstString newPath)
{
	thread_local wchar_t oldPathW[MAX_PATH];
	thread_local wchar_t newPathW[MAX_PATH];
	String::Utf8ToWide(oldPathW, MAX_PATH, oldPath);
	String::Utf8ToWide(newPathW, MAX_PATH, newPath);

	BOOL result = MoveFileExW(oldPathW, newPathW, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
	if (!result)
		AM_ERRF(L"fiDeviceLocal::Rename(oldName: %ls, newName: %ls) -> Failed to rename file, last error: %#x",
			oldPathW, newPathW, GetLastError());
	return result;
}

bool rage::fiDeviceLocal::MakeDirectory(ConstString path)
{
	ConstWString wPath = ConvertPathToWide(path);

	BOOL result = CreateDirectoryW(wPath, NULL);
	if (!result)
	{
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			return true;

		AM_ERRF(L"fiDeviceLocal::MakeDirectory(%ls) -> Failed to create directory, last error: %#x", wPath, GetLastError());
	}
	return result;
}

bool rage::fiDeviceLocal::UnmakeDirectory(ConstString path)
{
	ConstWString wPath = ConvertPathToWide(path);

	BOOL result = RemoveDirectoryW(wPath);
	if (!result)
		AM_ERRF(L"fiDeviceLocal::UnmakeDirectory(%ls) -> Failed to remove directory, last error: %#x", wPath, GetLastError());
	return result;
}

u64 rage::fiDeviceLocal::GetFileSize(ConstString path)
{
	ConstWString wPath = ConvertPathToWide(path);

	WIN32_FILE_ATTRIBUTE_DATA fileInfo{};
	if (!GetFileAttributesExW(wPath, GetFileExInfoStandard, &fileInfo))
	{
		AM_ERRF(L"fiDeviceLocal::GetFileSize(%ls) -> Failed to get file attributes, last error: %#x", wPath, GetLastError());
		return 0;
	}

	return TODWORD64(fileInfo.nFileSizeLow, fileInfo.nFileSizeHigh);
}

u64 rage::fiDeviceLocal::GetFileTime(ConstString path)
{
	ConstWString wPath = ConvertPathToWide(path);

	WIN32_FILE_ATTRIBUTE_DATA fileInfo{};
	if (!GetFileAttributesExW(wPath, GetFileExInfoStandard, &fileInfo))
	{
		AM_ERRF(L"fiDeviceLocal::GetFileTime(%ls) -> Failed to get file attributes, last error: %#x", wPath, GetLastError());
		return 0;
	}

	FILETIME time = fileInfo.ftLastWriteTime;
	return TODWORD64(time.dwLowDateTime, time.dwHighDateTime);
}

bool rage::fiDeviceLocal::SetFileTime(ConstString path, u64 time)
{
	HANDLE file = Open(path, false);
	if (file == FI_INVALID_HANDLE)
		return false;

	FILETIME fileTime{ LODWORD(time), HIDWORD(time) };
	BOOL result = ::SetFileTime(file, NULL, NULL, &fileTime);
	if (!result)
	{
		ConstWString wPath = ConvertPathToWide(path);

		AM_ERRF(L"fiDeviceLocal::SetFileTime(name: %ls, time: %llu) -> Failed to set time, last error: %#x",
			wPath, time, GetLastError());
	}

	return result;
}

fiHandle_t rage::fiDeviceLocal::FindFileBegin(ConstString path, fiFindData& findData)
{
	// NOTE: Native implementation forcefully adds '*' to the end of the string and it breaks
	// search in cases when we don't want to iterate directory but just find ... single file
	ConstWString wPath = ConvertPathToWide(path);

	WIN32_FIND_DATAW winFindData{};
	HANDLE file = FindFirstFileW(wPath, &winFindData);
	if (file == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
			return FI_INVALID_HANDLE;

		AM_ERRF(L"fiDeviceLocal::FindFileBegin(%ls) -> Failed, last error: %#x", wPath, GetLastError());
		return FI_INVALID_HANDLE;
	}
	Win32FindDataToFi(winFindData, findData);
	return file;
}

bool rage::fiDeviceLocal::FindFileNext(fiHandle_t file, fiFindData& findData)
{
	WIN32_FIND_DATAW winFindData{};
	BOOL result = FindNextFileW(file, &winFindData);
	if (!result)
	{
		if (GetLastError() == ERROR_NO_MORE_FILES)
			return false;

		AM_ERRF("fiDeviceLocal::FindFileNext(%p) -> Failed, last error: %#x", file, GetLastError());
		return FI_INVALID_HANDLE;
	}
	Win32FindDataToFi(winFindData, findData);
	return result;
}

bool rage::fiDeviceLocal::FindFileEnd(fiHandle_t file)
{
	BOOL result = FindClose(file);
	if (!result)
		AM_ERRF("fiDeviceLocal::SetFileTime(%p) -> Failed to close, last error: %#x", file, GetLastError());
	return result;
}

bool rage::fiDeviceLocal::SetEndOfFile(fiHandle_t file)
{
	BOOL result = ::SetEndOfFile(file);
	if (!result)
		AM_ERRF("fiDeviceLocal::SetEndOfFile(%p) -> Failed, last error: %#x", file, GetLastError());
	return result;
}

u32 rage::fiDeviceLocal::GetAttributes(ConstString path)
{
	ConstWString wPath = ConvertPathToWide(path);
	return GetFileAttributesW(wPath);
}

bool rage::fiDeviceLocal::SetAttributes(ConstString path, u32 attributes)
{
	ConstWString wPath = ConvertPathToWide(path);
	BOOL result = SetFileAttributesW(wPath, attributes);
	if (!result)
		AM_ERRF(L"fiDeviceLocal::SetAttributes(path: %ls, attributes: %u) -> Failed, last error: %#x",
			wPath, attributes, GetLastError());
	return result;
}
