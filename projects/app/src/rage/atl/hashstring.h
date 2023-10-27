//
// File: hashstring.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "string.h"
#include "common/types.h"
#include "rage/crypto/joaat.h"

namespace rage
{
	/**
	 * \brief Holds hash and sometimes original string.
	 */
	class atHashString
	{
		u32	m_Hash;
		atString m_String;
	public:
		atHashString() = default;
		atHashString(atString string)
		{
			m_Hash = joaat(string);
			m_String = std::move(string);
		}
		atHashString(u32 hash)
		{
			m_Hash = hash;
		}
		atHashString(ConstString string) : atHashString(atString(string)) {}

		ConstString GetCStr() const { return m_String; }
		u32 GetHash() const { return m_Hash; }

		operator u32() const { return m_Hash; }
	};
}
