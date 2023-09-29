#pragma once

#include "rage/atl/array.h"
#include "rage/atl/fixedarray.h"
#include "rage/atl/map.h"
#include "rage/atl/set.h"
#include "rage/atl/string.h"
#include "system/ptr.h"

namespace rageam
{
	// Dynamic array with 32 bit index
	template<typename T>
	using List = rage::atArray<T, u32>;

	// Dynamic array with 16 bit index
	template<typename T>
	using SmallList = rage::atArray<T, u16>;

	// Array with fixed capacity
	template<typename T, s32 Capacity>
	using FixedList = rage::atFixedArray<T, Capacity>;

	// List with shared pointers 
	template<typename T>
	using PList = List<amPtr<T>>;

	// List of unique pointers
	template<typename T>
	using UPList = List<amUniquePtr<T>>;

	// A dynamic ASCII string
	using string = rage::atString;

	// A dynamic UTF8 string
	using wstring = rage::atWideString;

	// Key - Value set
	template<typename TKey, typename TValue, typename THashFn = rage::atMapHashFn<TKey>>
	using Dictionary = rage::atMap<TKey, TValue, THashFn>;

	// Value set
	template<typename TKey>
	using HashSet = rage::atSet<TKey>;

	constexpr u32 Hash(ConstString str) { return rage::joaat(str); }
	constexpr u32 Hash(ConstWString str) { return rage::joaat(str); }
}
