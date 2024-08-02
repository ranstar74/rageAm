//
// File: stringwrapper.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <type_traits>

#include "am/string/char.h"
#include "am/system/asserts.h"
#include "common/types.h"

/**
 * \brief Wrapper on C-Style string that supports Ansi & Wide characters.
 */
template<typename TString>
class StringWrapper
{
	using TChar = std::remove_const_t<std::remove_pointer_t<TString>>;
	using TCString = std::add_const_t<std::remove_pointer_t<TString>>*;
	using Char = CharBase<TChar>;

	TString m_Str;
public:
	StringWrapper(TString str) : m_Str(str) {}
	StringWrapper();

	/**
	 * \brief Calculates length of the string until null-terminator character.
	 */
	u32 Length() const
	{
		u32 length = 0;
		while (m_Str[length]) { length++; }
		return length;
	}

	/**
	 * \brief Length + Null-Terminator.
	 */
	u32 Size() const
	{
		return Length() + 1;
	}

	/**
	 * \brief Attempts to find first (from left) occurrence of given character in this string.
	 */
	s32 IndexOf(TChar c, bool ignoreCase = false)
	{
		if (ignoreCase) c = Char::ToLower(c);

		s32 i = 0;
		while (m_Str[i++])
		{
			TChar a = m_Str[i];
			if (ignoreCase) a = Char::ToLower(a);
			if (a == c) return i;
		}
		return -1;
	}

	/**
	 * \brief Attempts to find first (from left) occurrence of character in given set in this string.
	 * \remarks Use example: s32 index = str.IndexOf<' ', '-', '_'>();
	 */
	template<TChar... TArgs>
	s32 IndexOf(bool ignoreCase = false)
	{
		TChar table[] = { TArgs... };
		if (ignoreCase) for (TChar& c : table) c = Char::ToLower(c);

		s32 i = 0;
		while (m_Str[i])
		{
			TChar a = m_Str[i];
			if (ignoreCase) a = Char::ToLower(a);

			for (TChar& c : table)
			{
				if (a == c) return i;
			}
			i++;
		}
		return -1;
	}

	/**
	 * \brief Attempts to find last (from right) occurrence of given character in this string.
	 */
	s32 LastIndexOf(TChar c, bool ignoreCase = false)
	{
		if (ignoreCase) c = Char::ToLower(c);

		s32 i = 0;	// Current index within string
		s32 k = -1;	// Last index where character matched
		while (m_Str[i++])
		{
			TChar a = m_Str[i];

			if (ignoreCase) a = Char::ToLower(a);
			if (a == c) k = i;
		}
		return k;
	}

	/**
	 * \brief Attempts to find last (from right) occurrence of character in given set in this string.
	 * \remarks Use example: s32 index = str.LastIndexOf<' ', '-', '_'>();
	 */
	template<TChar... TArgs>
	s32 LastIndexOf(bool ignoreCase = false)
	{
		TChar table[] = { TArgs... };
		if (ignoreCase)
		{
			for (TChar& c : table)
				c = Char::ToLower(c);
		}

		s32 i = 0;	// Current index within string
		s32 k = -1;	// Last index where character matched
		while (m_Str[i++])
		{
			TChar a = m_Str[i];
			if (ignoreCase) a = Char::ToLower(a);

			for (TChar c : table)
			{
				if (a == c) k = i;
			}
		}
		return k;
	}

	/**
	 * \brief Gets whether the first character in this string is equal to given one.
	 */
	bool StartsWith(TChar c, bool ignoreCase = false)
	{
		if (ignoreCase)
			return Char::ToLower(m_Str[0]) == Char::ToLower(c);
		return m_Str[0] == c;
	}

	/**
	 * \brief Gets whether the last character in this string is equal to given one.
	 */
	bool EndsWith(TChar c, bool ignoreCase = false)
	{
		u32 length = Length();
		if (length == 0) return false;
		u32 lastIndex = length - 1;
		if (ignoreCase)
			return Char::ToLower(m_Str[lastIndex]) == Char::ToLower(c);
		return m_Str[lastIndex] == c;
	}

	/**
	 * \brief Gets whether this string starts with given substring.
	 */
	bool StartsWith(TCString subString, bool ignoreCase = false)
	{
		u32 i = 0;
		while (true)
		{
			if (!subString[i])
				return true;

			TChar a = m_Str[i];
			TChar b = subString[i];
			if (ignoreCase)
			{
				a = Char::ToLower(a);
				b = Char::ToLower(b);
			}
			if (a != b) return false;
			++i;
		}
	}

	/**
	 * \brief Gets whether this string ends with given substring.
	 */
	bool EndsWith(TCString postfix, bool ignoreCase = false)
	{
		StringWrapper expectedPostfix = postfix;

		u32 length = Length();
		u32 postfixLength = expectedPostfix.Length();
		if (length < postfixLength)
			return false;

		StringWrapper actualPostfix = Substring(length - postfixLength);
		return expectedPostfix.Equals(actualPostfix, ignoreCase);
	}

	/**
	 * \brief Gets index of first (from left) occurrence of given substring in this string.
	 */
	s32 IndexOf(TCString substring, bool ignoreCase = false)
	{
		s32 i = 0;
		while (m_Str[i])
		{
			StringWrapper a = Substring(i);
			if (a.StartsWith(substring, ignoreCase))
				return i;

			++i;
		}
		return -1;
	}

	/**
	 * \brief Gets index of last (from right) occurrence of given substring in this string.
	 */
	s32 LastIndexOf(TCString substring, bool ignoreCase = false)
	{
		s32 k = -1;
		s32 i = 0;
		while (m_Str[i])
		{
			StringWrapper a = Substring(i);
			if (a.StartsWith(substring, ignoreCase))
				k = i;

			++i;
		}
		return k;
	}

	/**
	 * \brief Gets pointer on this string at given index (offset), index has to be within string range.
	 */
	StringWrapper Substring(u32 startIndex)
	{
		return &m_Str[startIndex];
	}

	/**
	 * \brief Compares this string with given one.
	 */
	bool Equals(StringWrapper other, bool ignoreCase = false)
	{
		if (!m_Str || !other.m_Str)
			return false;

		u32 i = 0;
		while (true)
		{
			TChar a = m_Str[i];
			TChar b = other[i];

			// All character matched and both strings ended
			if (!a && !b)
				return true;

			// One string ended but other didn't - they aren't equal
			if (!a || !b)
				return false;

			if (ignoreCase)
			{
				a = Char::ToLower(a);
				b = Char::ToLower(b);
			}

			// Some character didn't match (possibly null-terminator), they're not equal
			if (a != b)
				return false;

			++i;
		}
	}

	/**
	 * \brief Converts all characters in this string to upper case, string must be mutable! (non const).
	 */
	void ToUpper()
	{
		static_assert(!std::is_same<const char*, TString>() && !std::is_same<const wchar_t*, TString>(),
			"StringWrapper::ToUpper() -> String char type is const.");

		u32 i = 0;
		while (m_Str[i])
		{
			m_Str[i] = Char::ToUpper(m_Str[i]);
			++i;
		}
	}

	/**
	 * \brief Converts all characters in this string to lower case, string must be mutable! (non const).
	 */
	void ToLower()
	{
		static_assert(!std::is_same<const char*, TString>() && !std::is_same<const wchar_t*, TString>(),
			"StringWrapper::ToLower() -> String char type is const.");

		u32 i = 0;
		while (m_Str[i])
		{
			m_Str[i] = Char::ToLower(m_Str[i]);
			++i;
		}
	}

	/**
	 * \brief Gets whether this string contains given substring at least once.
	 */
	bool Contains(TCString other, bool ignoreCase = false)
	{
		return IndexOf(other, ignoreCase) != -1;
	}

	/**
	 * \brief Gets whether this string contains given character at least once.
	 */
	bool Contains(TChar other, bool ignoreCase = false)
	{
		return IndexOf(other, ignoreCase) != -1;
	}

	/**
	 * \brief Compares this string with the other.
	 * \remarks Comparison is case-sensitive, see ::Equals if you need to ignore case.
	 */
	bool operator==(StringWrapper other) { return Equals(other); }

	/**
	 * \brief Compares this string with the other.
	 * \remarks Comparison is case-sensitive, see ::Equals if you need to ignore case.
	 */
	bool operator==(TCString other) { return Equals(StringWrapper(other)); }

	/**
	 * \brief Gets view at this string at given offset.
	 */
	StringWrapper operator+(u32 delta)
	{
		return &m_Str[delta];
	}

	/**
	 * \brief Moves this string view by given offset.
	 */
	StringWrapper& operator+=(u32 delta)
	{
		m_Str = &m_Str[delta];
		return *this;
	}

	/**
	 * \brief Gets reference to character at given index, index has to be within string range.
	 */
	auto& operator[] (u32 index) const
	{
#ifdef DEBUG
		AM_ASSERT(index < Size(), "StringWrapper::operator[]() -> Index %u is out of range.", index);
#endif
		return m_Str[index];
	}
	operator TString() const { return m_Str; }
};

template<> inline StringWrapper<const char*>::StringWrapper() { m_Str = ""; }
template<> inline StringWrapper<const wchar_t*>::StringWrapper() { m_Str = L""; }

using MutableString = StringWrapper<char*>;
using MutableWString = StringWrapper<wchar_t*>;
using ImmutableString = StringWrapper<const char*>;
using ImmutableWString = StringWrapper<const wchar_t*>;
