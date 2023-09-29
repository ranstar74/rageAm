//
// File: hash.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/string/char.h"
#include "common/types.h"

namespace rage
{
	constexpr u32 ELFHashUpperCased(ConstString str)
	{
		// https://www.programmingalgorithms.com/algorithm/elf-hash/cpp/

		unsigned int hash = 0;
		unsigned int x;

		ConstString cursor = str;
		while (*cursor != '\0')
		{
			char ch = ToUpperLUT(*cursor);

			hash = (hash << 4) + ch;
			if ((x = hash & 0xF0000000) != 0)
			{
				hash ^= x >> 24;
			}
			hash &= ~x;

			cursor++;
		}

		return hash;
	}

	constexpr u16 crBoneHash(ConstString str)
	{
		u32 elfHash = ELFHashUpperCased(str);

		// Closest prime number to 65535 (UINT16_MAX)
		// (prime bucket sizes give less hash collisions)
		u16 hash = static_cast<u16>(elfHash % 65167);

		// Add remaining after wrapping
		hash += UINT16_MAX - 65167;

		return hash;
	}
}
