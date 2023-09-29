//
// File: win32.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>

#include "common/types.h"

#define LODWORD(qw)    ((DWORD)(qw))
#define HIDWORD(qw)    ((DWORD)(((qw) >> 32) & 0xffffffff))

#define TODWORD64(low, high) ((DWORD64)(high) << 32 | (DWORD64)(low))

#define SET_OVERLAPPED_OFFSET(ovp, qw) \
	(ovp).Offset = LODWORD((qw)); \
	(ovp).OffsetHigh = HIDWORD((qw))

enum FG_COLORS
{
	FG_BLACK = 0,
	FG_BLUE = 1,
	FG_GREEN = 2,
	FG_CYAN = 3,
	FG_RED = 4,
	FG_MAGENTA = 5,
	FG_BROWN = 6,
	FG_LIGHTGRAY = 7,
	FG_GRAY = 8,
	FG_LIGHTBLUE = 9,
	FG_LIGHTGREEN = 10,
	FG_LIGHTCYAN = 11,
	FG_LIGHTRED = 12,
	FG_LIGHTMAGENTA = 13,
	FG_YELLOW = 14,
	FG_WHITE = 15
};

enum BG_COLORS
{
	BG_NAVYBLUE = 16,
	BG_GREEN = 32,
	BG_TEAL = 48,
	BG_MAROON = 64,
	BG_PURPLE = 80,
	BG_OLIVE = 96,
	BG_SILVER = 112,
	BG_GRAY = 128,
	BG_BLUE = 144,
	BG_LIME = 160,
	BG_CYAN = 176,
	BG_RED = 192,
	BG_MAGENTA = 208,
	BG_YELLOW = 224,
	BG_WHITE = 240
};

LARGE_INTEGER ToLargeInteger(LONG64 value);
WORD SetConsoleColor(WORD value);
void GetModuleNameFromAddress(u64 address, char* buffer, u32 bufferSize);
bool CompressDirectory(const wchar_t* name);
void GetModuleBaseAndSize(DWORD64* lpBase, DWORD64* lpSize);
// Gets file extension name as it's shown in Windows explorer app.
void GetDisplayTypeName(wchar_t* dest, u32 destSize, const wchar_t* path, u32 attributes);
// Gets rageAm module.
HMODULE GetCurrentModule();
// Checks if current user has enough rights to access file / directory.
bool CanAccessPath(const wchar_t* path, DWORD accessRights);
// Buffer size has to be 4
void GetSystemDisk(wchar_t* buffer);
