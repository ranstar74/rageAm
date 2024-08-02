//
// File: fileutils.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/ptr.h"
#include "common/types.h"

#include <Windows.h>
#include <functional>

namespace rage
{
	struct fiPackHeader;
	struct datResourceInfo;
}

namespace rageam::file
{
	void EnumerateDirectory(ConstWString path, bool recurse, const std::function<void(const WIN32_FIND_DATAW&, ConstWString fullPath)>& findFn);

	struct FileBytes
	{
		amPtr<char> Data;
		u32			Size = 0;
	};

	bool IsFileExists(const char* path);
	bool IsFileExists(const wchar_t* path);
	HANDLE OpenFile(const wchar_t* path);
	HANDLE CreateNew(const wchar_t* path);
	u64 GetFileSize64(const wchar_t* path);
	u32 GetFileSize(const wchar_t* path);
	bool ReadAllBytes(const wchar_t* path, FileBytes& outFileBytes);
	bool IsDirectory(const wchar_t* path);
	u64 GetFileModifyTime(const wchar_t* path);

	// Stream I/O Helpers
	FILE* OpenFileStream(const wchar_t* path, const wchar_t* mode);
	size_t ReadFileStream(pVoid buffer, size_t bufferSize, size_t readSize, FILE* fs);
	bool WriteFileStream(pConstVoid buffer, size_t writeSize, FILE* fs);
	void CloseFileStream(FILE* file);

	// Rage resource information, returns version (0 if failed to open file or magic was invalid)
	u32 GetResourceInfo(const wchar_t* path, rage::datResourceInfo& info);
	bool GetPackfileHeader(const wchar_t* path, rage::fiPackHeader& header);

	struct FSHandle
	{
		FILE* fs;
		FSHandle(FILE* fs) { this->fs = fs; }
		~FSHandle()
		{
			if (fs) // Null pointer is not considered as valid stream
			{
				CloseFileStream(fs);
				fs = nullptr;
			}
		}

		FILE* Get() const { return fs; }

		bool operator!() const { return !fs; }
	};
}
