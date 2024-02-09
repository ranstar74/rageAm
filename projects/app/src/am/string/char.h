//
// File: char.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <cctype>
#include <cwctype>
#include <Windows.h>
#include <array>

#include "common/types.h"

template<typename TChar>
struct CharBase
{
	TChar Value;

	CharBase(TChar value) : Value(value) {}

	static bool IsDigit(TChar value);
	static bool IsLetter(TChar value);
	static bool IsWhiteSpace(TChar value);
	static bool IsPathSeparator(TChar value);
	static bool IsUpper(TChar value);
	static bool IsLower(TChar value);

	static TChar PathSeparator();

	static TChar ToUpper(TChar value);
	static TChar ToLower(TChar value);

	CharBase& operator=(TChar value) { Value = value; return *this; }
	operator TChar() const { return Value; }
};

// Char

template<> inline bool CharBase<char>::IsDigit(char value) { return isdigit(value); }
template<> inline bool CharBase<char>::IsLetter(char value) { return isalpha(value); }
template<> inline bool CharBase<char>::IsWhiteSpace(char value) { return isblank(value); }
template<> inline bool CharBase<char>::IsPathSeparator(char value) { return value == '/' || value == '\\'; }
template<> inline bool CharBase<char>::IsUpper(char value) { return isupper(value); }
template<> inline bool CharBase<char>::IsLower(char value) { return islower(value); }

template<> inline char CharBase<char>::PathSeparator() { return '\\'; }
template<> inline char CharBase<char>::ToUpper(char value) { return (char)toupper(value); }
template<> inline char CharBase<char>::ToLower(char value) { return (char)tolower(value); }

// Wide char

template<> inline bool CharBase<wchar_t>::IsDigit(wchar_t value) { return iswdigit(value); }
template<> inline bool CharBase<wchar_t>::IsLetter(wchar_t value) { return iswalpha(value); }
template<> inline bool CharBase<wchar_t>::IsWhiteSpace(wchar_t value) { return iswblank(value); }
template<> inline bool CharBase<wchar_t>::IsPathSeparator(wchar_t value) { return value == L'/' || value == L'\\'; }
template<> inline bool CharBase<wchar_t>::IsUpper(wchar_t value) { return IsCharUpperW(value); }
template<> inline bool CharBase<wchar_t>::IsLower(wchar_t value) { return IsCharLowerW(value); }

template<> inline wchar_t CharBase<wchar_t>::PathSeparator() { return L'\\'; }

// towlower & towupper don't support unicode so use WinApi's CharUpperW and CharLowerW
// If the high-order word of this parameter is zero, the low-order word must contain a single character to be converted.

#pragma warning (push)
#pragma warning (disable : 4302) // type cast': truncation from 'LPWSTR' to 'wchar_t'
template<> inline wchar_t CharBase<wchar_t>::ToUpper(wchar_t value) { return (wchar_t)CharUpperW((LPWSTR)value); } // NOLINT(clang-diagnostic-int-to-pointer-cast, clang-diagnostic-pointer-to-int-cast)
template<> inline wchar_t CharBase<wchar_t>::ToLower(wchar_t value) { return (wchar_t)CharLowerW((LPWSTR)value); } // NOLINT(clang-diagnostic-int-to-pointer-cast, clang-diagnostic-pointer-to-int-cast)
#pragma warning (pop)

using Char = CharBase<char>;
using WChar = CharBase<wchar_t>;

// Compile-time ToUpper & ToLower

constexpr auto ToUpperLUTArray()
{
	std::array<char, 255> lookup;
	for (u8 i = 0; i < 255; i++)
	{
		char c = i >= 'a' && i <= 'z' ? char(i - ('a' - 'A')) : char(i);
		lookup[i] = c;
	}
	return lookup;
}

constexpr auto ToLowerLUTArray()
{
	std::array<char, 255> lookup;
	for (u8 i = 0; i < 255; i++)
	{
		char c = i >= 'A' && i <= 'Z' ? char(i + ('a' - 'A')) : char(i);
		lookup[i] = c;
	}
	return lookup;
}

constexpr char ToUpperLUT(char c)
{
	constexpr auto lookup = ToUpperLUTArray();
	return char(lookup[c]);
}

constexpr char ToLowerLUT(char c)
{
	constexpr auto lookup = ToLowerLUTArray();
	return char(lookup[c]);
}
