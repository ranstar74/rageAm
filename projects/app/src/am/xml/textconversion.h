#pragma once

#include <cstdio>

#include "rage/physics/archetype.h"

#define XML_FLOAT_FMT "%g"

template<typename T>
ConstString ToString(const T& value)
{
	static char buffer[128];

	auto print = [&](ConstString fmt)
		{
			sprintf_s(buffer, sizeof buffer, fmt, value); // NOLINT(clang-diagnostic-double-promotion)
			return buffer;
		};

	if constexpr (std::is_same_v<T, ConstString>) { return value; }
	if constexpr (std::is_same_v<T, float>) { return print(XML_FLOAT_FMT); }
	if constexpr (std::is_same_v<T, double>) { return print(XML_FLOAT_FMT); }
	if constexpr (std::is_same_v<T, bool>) { return value ? "true" : "false"; }
	if constexpr (std::is_same_v<T, u8>) { return print("%u"); }
	if constexpr (std::is_same_v<T, s8>) { return print("%i"); }
	if constexpr (std::is_same_v<T, u16>) { return print("%u"); }
	if constexpr (std::is_same_v<T, s16>) { return print("%i"); }
	if constexpr (std::is_same_v<T, u32>) { return print("%u"); }
	if constexpr (std::is_same_v<T, s32>) { return print("%i"); }
	if constexpr (std::is_same_v<T, u64>) { return print("%llu"); }
	if constexpr (std::is_same_v<T, s64>) { return print("%lli"); }

	return "UNSUPPORTED";
}

template<typename T>
bool FromString(ConstString str, T& outValue)
{
	auto scan = [&](ConstString fmt)
		{
			return sscanf_s(str, fmt, &outValue) > 0;
		};

	if constexpr (std::is_same_v<T, ConstString>) { outValue = str; return true; }
	if constexpr (std::is_same_v<T, float>) { return scan(XML_FLOAT_FMT); }
	if constexpr (std::is_same_v<T, double>) { return scan(XML_FLOAT_FMT); }
	if constexpr (std::is_same_v<T, bool>) { outValue = _stricmp(str, "true") == 0 ? true : false; return true; }
	if constexpr (std::is_same_v<T, u8>) { return scan("%u"); }
	if constexpr (std::is_same_v<T, s8>) { return scan("%i"); }
	if constexpr (std::is_same_v<T, u16>) { return scan("%u"); }
	if constexpr (std::is_same_v<T, s16>) { return scan("%i"); }
	if constexpr (std::is_same_v<T, u32>) { return scan("%u"); }
	if constexpr (std::is_same_v<T, s32>) { return scan("%i"); }
	if constexpr (std::is_same_v<T, u64>) { return scan("%llu"); }
	if constexpr (std::is_same_v<T, s64>) { return scan("%lli"); }

	return false;
}
