//
// File: flagset.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <type_traits>

template<typename T>
struct FlagSet
{
	using TSize = std::underlying_type_t<T>;

	TSize Value = static_cast<TSize>(0);

	FlagSet() = default;
	FlagSet(TSize value) { Value = value; }

	bool IsSet(T flag) const { return Value & flag; }
	bool IsSet(TSize flags) const { return Value & flags; }
	void Set(T flag, bool on)
	{
		Value &= ~flag;
		if (on) Value |= flag;
	}
};
