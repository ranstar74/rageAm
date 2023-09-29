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
#include "rage/atl/string.h"

/**
 * \brief Formats size (in bytes) with appropriate units (Bytes, KB, MB, GB),
 * and writes to given buffer in format '512 Bytes', '1.21 KB', '1.21 MB', '1.21 GB'
 */
template<typename TSize>
void FormatBytes(char* buffer, u32 bufferSize, TSize size)
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

template<typename TSize>
rage::atString FormatBytes(TSize size)
{
	char buffer[16];
	FormatBytes(buffer, 16, size);
	return rage::atString(buffer);
}
