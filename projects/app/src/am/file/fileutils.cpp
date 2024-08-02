#include "fileutils.h"

#include "am/system/asserts.h"
#include "path.h"
#include "helpers/win32.h"
#include "rage/paging/resourceheader.h"
#include "rage/file/packfile.h"

void rageam::file::EnumerateDirectory(ConstWString path, bool recurse, const std::function<void(const WIN32_FIND_DATAW&, ConstWString fullPath)>& findFn)
{
	WPath searchPath = path;
	searchPath /= L"*";

	WIN32_FIND_DATAW findData;
	HANDLE           findHandle = FindFirstFileW(searchPath, &findData);
	if (findHandle == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (!wcscmp(findData.cFileName, L".") || !wcscmp(findData.cFileName, L"..") || findData.dwFileAttributes == INVALID_FILE_ATTRIBUTES)
			continue;

		WPath findPath = path;
		findPath /= findData.cFileName;
		findFn(findData, findPath);
		if (recurse && findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			EnumerateDirectory(findPath, true, findFn);
	} while (FindNextFileW(findHandle, &findData));
	FindClose(findHandle);
}

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

u32 rageam::file::GetFileSize(const wchar_t* path)
{
	return static_cast<u32>(GetFileSize64(path));
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

bool rageam::file::IsDirectory(const wchar_t* path)
{
	return GetFileAttributesW(path) & FILE_ATTRIBUTE_DIRECTORY;
}

u64 rageam::file::GetFileModifyTime(const wchar_t* path)
{
	if (!IsFileExists(path))
		return false;

	HANDLE hFile = OpenFile(path);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	FILETIME modifyTime = {};
	GetFileTime(hFile, NULL, NULL, &modifyTime);

	CloseHandle(hFile);

	return TODWORD64(modifyTime.dwLowDateTime, modifyTime.dwHighDateTime);
}

FILE* rageam::file::OpenFileStream(const wchar_t* path, const wchar_t* mode)
{
	FILE* file = nullptr;
	(void)_wfopen_s(&file, path, mode);
	return file;
}

size_t rageam::file::ReadFileStream(pVoid buffer, size_t bufferSize, size_t readSize, FILE* fs)
{
	return fread_s(buffer, bufferSize, sizeof(char), readSize, fs);
}

bool rageam::file::WriteFileStream(pConstVoid buffer, size_t writeSize, FILE* fs)
{
	return fwrite(buffer, sizeof(char), writeSize, fs) == writeSize;
}

void rageam::file::CloseFileStream(FILE* file)
{
	fclose(file);
}

u32 rageam::file::GetResourceInfo(const wchar_t* path, rage::datResourceInfo& info)
{
	info = {};
	FILE* stream = OpenFileStream(path, L"rb");
	if (!stream)
		return 0;
	u32 version = 0;
	rage::datResourceHeader header;
	ReadFileStream(&header, 16, 16, stream);
	if (header.IsValidMagic())
	{
		version = header.Version;
		info = header.Info;
	}
	CloseFileStream(stream);
	return version;
}

bool rageam::file::GetPackfileHeader(const wchar_t* path, rage::fiPackHeader& header)
{
	header = {};
	FILE* stream = OpenFileStream(path, L"rb");
	if (!stream)
		return false;
	ReadFileStream(&header, 16, 16, stream);
	CloseFileStream(stream);
	return header.Magic == rage::fiPackHeader::MAGIC;
}
