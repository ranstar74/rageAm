//
// File: format.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <cstdio>

#include "common/types.h"

// Formats size (in bytes) with appropriate units (Bytes, KB, MB, GB),
// and writes to given buffer in format '512 Bytes', '1.21 KB', '1.21 MB', '1.21 GB'
template<typename TSize>
void FormatSize(char* buffer, u32 bufferSize, TSize size)
{
	u64 size64 = static_cast<u64>(size);
	float sizeF = static_cast<float>(size);

	if (size < 1024ull)
		sprintf_s(buffer, bufferSize, "%llu Bytes", size64);
	else if (size < 1024ull * 1024ull)
		sprintf_s(buffer, bufferSize, "%.2f KB", sizeF / 1024.0f);
	else if (size < 1024ull * 1024ull * 1024ull)
		sprintf_s(buffer, bufferSize, "%.2f MB", sizeF / 1024.0f / 1024.0f);
	else
		sprintf_s(buffer, bufferSize, "%.2f GB", sizeF / 1024.0f / 1024.0f / 1024.0f);
}

// Formats into thread-local buffer, holds 16 internal buffers
// allowing to use this function multiple times without copying returned string
template<typename TSize>
ConstString FormatSize(TSize size)
{
	// Largest formatted string shouldn't exceed 12 characters
	// 1 048 576 KB
	static constexpr int MAX_BUFFERS = 16;
	static constexpr int BUFFER_SIZE = 14; 
	thread_local char buffers[MAX_BUFFERS][BUFFER_SIZE];
	thread_local int currentBuffer = 0;

	char* buffer = buffers[currentBuffer];
	FormatSize(buffer, BUFFER_SIZE, size);
	currentBuffer = (currentBuffer + 1) % MAX_BUFFERS;
	return buffer;
}
