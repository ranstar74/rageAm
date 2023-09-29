#include "fileutils.h"

#include "am/system/asserts.h"

bool rageam::file::IsFileExists(const char* path)
{
	return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

bool rageam::file::IsFileExists(const wchar_t* path)
{
	return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

HANDLE rageam::file::OpenFile(const wchar_t* path)
{
	return CreateFileW(
		path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

HANDLE rageam::file::CreateNew(const wchar_t* path)
{
	return CreateFileW(
		path, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

u64 rageam::file::GetFileSize64(const wchar_t* path)
{
	HANDLE hFile = OpenFile(path);
	if (!AM_VERIFY(hFile != INVALID_HANDLE_VALUE,
		L"GetFileSize64() -> Failed to open file %ls, Last error: %u", path, GetLastError()))
		return 0;

	LARGE_INTEGER fileSize;
	if (!AM_VERIFY(GetFileSizeEx(hFile, &fileSize),
		L"GetFileSize64() -> Failed to get file size %ls, Last error: %u", path, GetLastError()))
	{
		CloseHandle(hFile);
		return 0;
	}

	CloseHandle(hFile);
	return fileSize.QuadPart;
}

bool rageam::file::ReadAllBytes(const wchar_t* path, FileBytes& outFileBytes)
{
	if (!AM_VERIFY(IsFileExists(path), L"ReadAllBytes() -> File at path %ls doesn't exists.", path))
		return false;

	HANDLE hFile = OpenFile(path);
	if (!AM_VERIFY(hFile != INVALID_HANDLE_VALUE, L"ReadAllBytes() -> Unable to open file at path %ls.", path))
		return false;

	u64 fileSize;
	if (!AM_VERIFY(GetFileSizeEx(hFile, (PLARGE_INTEGER)&fileSize),
		L"ReadAllBytes() -> Failed to get file size %ls, Last error: %u", path, GetLastError()))
	{
		CloseHandle(hFile);
		return false;
	}

	DWORD bytesReaded;
	char* data = static_cast<char*>(operator new(fileSize));
	if (!AM_VERIFY(ReadFile(hFile, data, static_cast<u32>(fileSize), &bytesReaded, NULL), 
		L"ReadAllBytes() -> Failed to read file %ls, Last error: %u", path, GetLastError()))
	{
		CloseHandle(hFile);
		return false;
	}

	outFileBytes.Data = amPtr<char>(data);
	outFileBytes.Size = static_cast<u32>(fileSize);

	CloseHandle(hFile);

	return true;
}
