//
// File: streamingdefs.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

namespace rage
{
	// NOTE:
	// - strLocalIndex is local index in strStreamingModule (simply index in array/pool)
	// - strIndex is global index in strStreamingEngine::ms_info array

	using strIndex_t = s32;

	static constexpr strIndex_t INVALID_STR_INDEX = -1;

	// strIndex and strLocalIndex are separate types for:
	// - Compatibility with rage code
	// - Preventing implicit cast between them
	// The only difference between those declarations is the class name
	
	class strIndex
	{
		strIndex_t m_Index = INVALID_STR_INDEX;

	public:
		strIndex() = default;
		explicit strIndex(strIndex_t index) : m_Index(index) {}

		strIndex_t Get() const { return m_Index; }
		void       Set(strIndex_t index) { m_Index = index; }
		bool       IsValid() const { return m_Index != INVALID_STR_INDEX; }

		bool operator==(const strIndex& other) const = default;
		bool operator==(const strIndex_t& other) const { return m_Index == other; }
	};

	class strLocalIndex
	{
		strIndex_t m_Index = INVALID_STR_INDEX;

	public:
		strLocalIndex() = default;
		strLocalIndex(strIndex_t index) : m_Index(index) {}

		strIndex_t Get() const { return m_Index; }
		void       Set(strIndex_t index) { m_Index = index; }
		bool       IsValid() const { return m_Index != INVALID_STR_INDEX; }

		bool operator==(const strLocalIndex& other) const = default;
		bool operator==(const strIndex_t& other) const { return m_Index == other; }
	};
}
