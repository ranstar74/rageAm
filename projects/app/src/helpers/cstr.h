//
// File: cstr.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

// Compile-time string extensions
namespace cstr
{
	constexpr size_t strlen(const char* str)
	{
		size_t len = 0;
		while (str[len])
			len++;
		return len;
	}

	constexpr const char* strrchr(const char* str, char c)
	{
		size_t len = strlen(str);
		while (true)
		{
			len--;

			if (str[len] == c)
				return str + len;

			if (len == 0) return nullptr;
		}
	}

	constexpr char tolower(char c)
	{
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		return c;
	}

	constexpr char toupper(char c)
	{
		if (c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		return c;
	}

	constexpr wchar_t towlower(wchar_t c) // Only ASCII set
	{
		if (c >= L'A' && c <= L'Z')
			c += L'a' - L'A';
		return c;
	}

	constexpr wchar_t towupper(wchar_t c) // Only ASCII set
	{
		if (c >= L'a' && c <= L'z')
			c += L'A' - L'a';
		return c;
	}
}
