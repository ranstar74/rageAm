//
// File: hashstring.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "helpers/cstr.h"

namespace rage
{
	using atHashValue = u32;

	// Jenkins one at a time hash
	// Template isn't used because it will break implicit cast operators

	constexpr u32 atStringHash(const char* str, bool lowerCase = true, u32 seed = 0)
	{
		u32 i = 0;
		u32 hash = seed;
		while (str[i] != '\0') {
			hash += lowerCase ? cstr::tolower(str[i++]) : str[i++];
			hash += hash << 10;
			hash ^= hash >> 6;
		}
		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;
		return hash;
	}

	constexpr u32 atStringHash(const wchar_t* str, bool lowerCase = true, u32 seed = 0)
	{
		u32 i = 0;
		u32 hash = seed;
		while (str[i] != '\0') {
			hash += lowerCase ? cstr::towlower(str[i++]) : str[i++];
			hash += hash << 10;
			hash ^= hash >> 6;
		}
		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;
		return hash;
	}

	constexpr u32 atStringViewHash(const char* str, int length, bool lowerCase = true, u32 seed = 0)
	{
		u32 hash = seed;
		for (int i = 0; i < length && str[i] != '\0'; i++) {
			hash += lowerCase ? cstr::tolower(str[i]) : str[i];
			hash += hash << 10;
			hash ^= hash >> 6;
		}
		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;
		return hash;
	}

	// Although it's not very good idea to compare hash to '0' but this is what rage does
	inline bool atIsNullHash(atHashValue hash) { return hash == 0 || hash == atStringHash("null"); }

	class atHashString
	{
		u32 m_Hash;

	public:
		atHashString() = default;
		atHashString(ConstString str) { m_Hash = atStringHash(str); }
		atHashString(u32 hash) { m_Hash = hash; }

		u32 GetHash() const { return m_Hash; }
		operator u32() const { return m_Hash; }

		ConstString TryGetCStr() const { return ""; }

		static atHashString Null() { return atHashString(); }
	};
}
