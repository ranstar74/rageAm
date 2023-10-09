//
// File: bitarray.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/system/asserts.h"
#include "am/system/ptr.h"
#include "common/types.h"

/**
 * \brief Allows to store large amount of bool flags with 1/8 of size comparing to bool[size].
 */
class BitArray
{
protected:
	u8* m_Bytes;
	u32 m_BytesCount;
	amUniquePtr<u8[]> m_Heap; // In case of heap allocation

	// Library ceil function is not constexpr
	static constexpr u32 CeilU32(float value)
	{
		const u32 i = static_cast<u32>(value);
		return value > static_cast<float>(i) ? i + 1u : i;
	}

	static constexpr u32 GetNumBytesForSize(u32 size)
	{
		return CeilU32(static_cast<float>(size) / 8.0f); // 8 bits in 1 byte
	}

	void GetLocation(u32 index, u32& byte, u32& bitMask) const
	{
		byte = index >> 3;
		bitMask = 1 << (index & 0x7);

#ifdef DEBUG
		AM_ASSERT(byte < m_BytesCount, "BoolMap::GetLocation() -> Index %u out of array size %u.", byte, m_BytesCount);
#endif
	}

	void InitWithHeap(u8* bytes, u32 size)
	{
		m_Bytes = bytes;
		m_BytesCount = size;
		memset(m_Bytes, 0, size);
	}

public:
	BitArray() = default;
	BitArray(u8* bytes, u32 size)
	{
		InitWithHeap(bytes, size);
	}

	void Set(u32 index, bool state = true) const
	{
		u32 byte, bitMask;
		GetLocation(index, byte, bitMask);
		m_Bytes[byte] &= ~bitMask;
		if (state) m_Bytes[byte] |= bitMask;
	}

	void Unset(u32 index) const { Set(index, false); }

	bool IsSet(u32 index) const
	{
		u32 byte, bitMask;
		GetLocation(index, byte, bitMask);
		return m_Bytes[byte] & bitMask;
	}

	void Allocate(u32 boolCount)
	{
		u32 numBytes = GetNumBytesForSize(boolCount);
		m_Heap = amUniquePtr<u8[]>(new u8[numBytes]);
		InitWithHeap(m_Heap.get(), numBytes);
	}
};

/**
 * \brief Allows to store large amount of bool flags with 1/8 of size comparing to bool[TSize], allocated on stack.
 * \tparam TSize Size of the bool array.
 */
template<u32 TSize>
class BitStackArray : public BitArray
{
	static_assert(TSize != 0);

	// For 12 bool map we have to allocate 2 unsigned bytes (16 bits)
	static constexpr u32 NUM_BYTES = GetNumBytesForSize(TSize);

	u8 m_ByteHeap[NUM_BYTES]{}; // NOLINT(clang-diagnostic-zero-length-array)
public:
	BitStackArray() : BitArray(m_ByteHeap, TSize)
	{

	}
};
