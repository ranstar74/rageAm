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
	bool ReadAllBytes(const wchar_t* path, FileBytes& outFileBytes);
}
