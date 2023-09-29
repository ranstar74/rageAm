#pragma once

#include "common/types.h"

namespace rage
{
	inline u32 atPartialDataHash(const void* buffer, size_t size, u32 seed = 0)
	{
		u32 hash = seed;
		for (size_t i = 0; i < size; i++)
		{
			hash += static_cast<const char*>(buffer)[i];
			hash += hash << 10;
			hash ^= hash >> 6;
		}
		return hash;
	}

	// Same as string hash function, can be used on any memory as crc32/md5 alternative
	inline u32 atDataHash(const void* buffer, size_t size, u32 seed = 0)
	{
		u32 hash = atPartialDataHash(buffer, size, seed);
		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;
		return hash;
	}
}
