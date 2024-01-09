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

namespace rageam::file
{
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
	size_t ReadFileSteam(pVoid buffer, size_t bufferSize, size_t readSize, FILE* fs);
	bool WriteFileSteam(pConstVoid buffer, size_t writeSize, FILE* fs);
	void CloseFileStream(FILE* file);

	struct FSHandle
	{
		FILE* fs;
		FSHandle(FILE* fs) { this->fs = fs; }
		~FSHandle() { CloseFileStream(fs); fs = nullptr; }

		FILE* Get() const { return fs; }

		bool operator!() const { return !fs; }
	};
}
