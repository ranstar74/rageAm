//
// File: string.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/system/ipc.h"
#include "am/string/stringwrapper.h"
#include "am/string/string.h"
#include "am/string/char.h"
#include "rage/atl/array.h"
#include "helpers/resharper.h"
#include "helpers/ranges.h"
#include "rage/atl/array.h"

namespace rage
{
	/**
	 * \brief Base class for ANSI and Unicode strings, see atString, atWideString;
	 * \tparam TChar Type of string character (char / wchar_t).
	 * \tparam TSize Size of string buffer.
	 */
	template<typename TChar, typename TSize = u16>
	class atBaseString : atArray<TChar, TSize>
	{
		using Char = CharBase<TChar>;
		using ConstString = const TChar*;
		using Buffer = atArray<TChar, TSize>;

		using Buffer::atArray;

		// Inserts given string at the end, trims string to given maximum length
		void Append(const TChar* str, TSize length)
		{
			TSize currentLength = GetLength();
			TSize totalLength = currentLength + length;
			TSize newSize = totalLength + 1; // Including null-terminator

			Buffer::VerifyBufferCanFitOrGrow(newSize);
			String::Copy(Buffer::m_Items + currentLength, newSize, str, length);
			Buffer::m_Size = newSize;
			Buffer::m_Items[totalLength] = '\0';
		}

		// Inserts given string at the end
		void Append(ConstString str)
		{
			Append(str, String::Length(str));
		}

		// Replaces current string with other string, trims string to given maximum length
		void Set(const TChar* str, TSize length)
		{
			if (!str)
				return;

			TSize size = length + 1; // Including null-terminator

			Buffer::VerifyBufferCanFitOrGrow(size);
			String::Copy(Buffer::m_Items, size, str, length);
			Buffer::m_Size = size;
		}

		// Replaces current string with other string
		void Set(ConstString str)
		{
			if (!str)
				return;
			Set(str, static_cast<TSize>(StringWrapper(str).Length()));
		}
	public:
		// Constructs empty string "" and reserves internal buffer to given length
		atBaseString(TSize length = 0) : Buffer(length + 1 /* Including null-terminator */) { Set(String::Empty<TChar>()); }
		// Copies from given string
		atBaseString(ConstString str) : Buffer(0) { Set(str); }
		// Copies from given string with maximum length specified.
		atBaseString(ConstString str, TSize hintLength)
		{
			if (!str) 
				return;
			TSize length = MIN(String::Length(str), hintLength);
			Set(str, length);
		}
		atBaseString(const atBaseString& other) : atBaseString(other.GetCStr(), other.GetLength()) { }
		atBaseString(atBaseString&& other) noexcept
		{
			std::swap(Buffer::m_Items, other.m_Items);
			std::swap(Buffer::m_Size, other.m_Size);
			std::swap(Buffer::m_Capacity, other.m_Capacity);
		}
		atBaseString(const std::string& str) : atBaseString(str.begin(), str.end()) {}
		atBaseString(const std::wstring& str) : atBaseString(str.begin(), str.end()) {}
		template<typename TIterator>
		atBaseString(TIterator start, TIterator end) : Buffer(0)
		{
			TSize length = end - start;
			TSize size = length + 1; // Including null-terminator.
			Buffer::VerifyBufferCanFitOrGrow(size);
			for (TSize i = 0; start < end; ++start, ++i)
				Buffer::m_Items[i] = *start;
			Buffer::m_Size = size;
			Buffer::m_Items[length] = '\0';
		}

		/**
		 * \brief Gets string length including null-terminator (size of internal buffer).
		 */
		TSize GetSize() const { return Buffer::GetSize(); }

		/**
		 * \brief Gets size of allocated internal buffer.
		 */
		TSize GetCapacity() const { return Buffer::GetCapacity(); }

		/**
		 * \brief Gets string length.
		 */
		TSize GetLength() const { return Buffer::GetSize() - 1; }

		/**
		 * \brief Resizes internal buffer to given string length.
		 */
		void Reserve(TSize length) { Buffer::Reserve(length + 1 /* null-terminator */); }

		/**
		 * \brief Resizes internal buffer to actually used size.
		 */
		void Shrink() { Buffer::Shrink(); }

		/**
		 * \brief Gets pointer to underlying const char array.
		 */
		const TChar* GetCStr() const { return Buffer::m_Items; }

		void AppendFormatVA(ConstString fmt, const va_list& args)
		{
			static sysCriticalSectionToken criticalSection;
			static TChar buffer[2048];

			sysCriticalSectionLock lock(criticalSection);

			String::FormatVA(buffer, 2048, fmt, args);
			operator+=(buffer);
		}

		PRINTF_ATTR(2, 3) void AppendFormat(ConstString fmt, ...)
		{
			va_list args;
			va_start(args, fmt);
			AppendFormatVA(fmt, args);
			va_end(args);
		}

		/**
		 * \brief Returns a new string with every occurrence of oldValue replaced to newValue.
		 */
		atBaseString Replace(ConstString oldValue, ConstString newValue, bool ignoreCase = false)
		{
			atBaseString result(GetLength());

			// @token@@token@token@@
			StringWrapper str = GetCStr();

			TSize oldValueLength = StringWrapper(oldValue).Length();
			TSize newValueLength = StringWrapper(newValue).Length();

			s32 nextOldIndex;
			while ((nextOldIndex = str.IndexOf(oldValue, ignoreCase)) != -1)
			{
				// If we have anything before old value
				if (nextOldIndex != 0)
					result.Append(str, static_cast<TSize>(nextOldIndex));

				// Replace it with new value
				result.Append(newValue, newValueLength);

				// Move past old value
				str += nextOldIndex + oldValueLength;
			}
			// Add remaining part
			result.Append(str);

			return result;
		}

		/**
		 * \brief Gets index of first (from left) occurrence of given substring in this string.
		 */
		s32 IndexOf(const TChar* substring, bool ignoreCase = false) const
		{
			StringWrapper str = GetCStr();
			return str.IndexOf(substring, ignoreCase);
		}

		/**
		 * \brief Gets index of last (from right) occurrence of given substring in this string.
		 */
		s32 LastIndexOf(const TChar* substring, bool ignoreCase = false) const
		{
			StringWrapper str = GetCStr();
			return str.LastIndexOf(substring, ignoreCase);
		}

		/**
		 * \brief Attempts to find first (from left) occurrence of given character in this string.
		 */
		s32 IndexOf(TChar c, bool ignoreCase = false) const
		{
			StringWrapper str = GetCStr();
			return str.IndexOf(c, ignoreCase);
		}

		/**
		 * \brief Attempts to find last (from right) occurrence of given character in this string.
		 */
		s32 LastIndexOf(TChar c, bool ignoreCase = false) const
		{
			StringWrapper str = GetCStr();
			return str.LastIndexOf(c, ignoreCase);
		}

		/**
		 * \brief Attempts to find last (from right) occurrence of character in given set in this string.
		 * \remarks Use example: s32 index = str.LastIndexOf<' ', '-', '_'>();
		 */
		template<TChar... TArgs>
		s32 LastIndexOf(bool ignoreCase = false)
		{
			StringWrapper str = GetCStr();
			return str.template LastIndexOf<TArgs...>(ignoreCase);
		}

		/**
		 * \brief Attempts to find first (from left) occurrence of character in given set in this string.
		 * \remarks Use example: s32 index = str.IndexOf<' ', '-', '_'>();
		 */
		template<TChar... TArgs>
		s32 IndexOf(bool ignoreCase = false)
		{
			StringWrapper str = GetCStr();
			return str.template IndexOf<TArgs...>(ignoreCase);
		}

		/**
		 * \brief Returns a new string that starts from given index, can be used in pair with IndexOf / LastIndexOf.
		 */
		atBaseString Substring(s32 startIndex)
		{
			AM_ASSERT(startIndex >= 0, startIndex < GetLength(),
				"atString::Substring(%u) -> Start index is outside of string bounds.", startIndex);
			return atBaseString(GetCStr() + startIndex);
		}

		/**
		 * \brief Returns a new string that starts from given index, can be used in pair with IndexOf / LastIndexOf.
		 * \n Length of substring will be clamped to actual string size bounds.
		 */
		atBaseString Substring(s32 startIndex, TSize length)
		{
			AM_ASSERT(startIndex >= 0,
				"atString::Substring(%u, %u) -> Start index is outside of string bounds.", startIndex, length);
			return atBaseString(GetCStr() + startIndex, length);
		}

		/**
		 * \brief Returns a new string with whitespaces removed from beginning and end.
		 */
		atBaseString Trim()
		{
			TSize trimStart = 0;
			for (; trimStart < GetLength(); ++trimStart)
			{
				if (!IsSpace(operator[](trimStart)))
					break;
			}

			atBaseString result(GetCStr() + trimStart);

			TSize length = result.GetLength();
			for (; length > 0; --length)
			{
				if (!IsSpace(result[length - 1]))
					break;
			}

			result.Reserve(length);
			result[length] = '\0';

			return result;
		}

		/**
		 * \brief Returns a new string with all characters converted to UPPERCASE.
		 */
		atBaseString ToUppercase() const
		{
			atBaseString result(*this);
			for (TChar& c : result)
				c = Char::ToUpper(c);
			return result;
		}

		/**
		 * \brief Returns a new string with all characters converted to lowercase.
		 */
		atBaseString ToLowercase() const
		{
			atBaseString result(*this);
			for (TChar& c : result)
				c = Char::ToLower(c);
			return result;
		}

		/**
		 * \brief Returns a new string with characters order reversed.
		 */
		atBaseString Reverse() const
		{
			atBaseString result(*this);

			auto start = result.begin();
			auto end = result.end();

			--end; // We don't want null-terminator to be swapped

			for (; start < end; ++start, --end)
				std::swap(*start, *end);

			return result;
		}

		/**
		 * \brief Returns whether a string occurs within this string at least once.
		 */
		bool Contains(ConstString substring, bool ignoreCase = false)
		{
			return IndexOf(substring, ignoreCase) != -1;
		}

		/**
		 * \brief Returns whether an character occurs within this string at least once.
		 */
		bool Contains(TChar c, bool ignoreCase = false)
		{
			return IndexOf(c, ignoreCase) != -1;
		}

		/**
		 * \brief Returns whether this string ends with a string (Suffix).
		 */
		bool EndsWith(ConstString postfix, bool ignoreCase = false) const
		{
			StringWrapper str = GetCStr();
			return str.EndsWith(postfix);
		}

		/**
		 * \brief Returns whether this string starts with given a string (Prefix).
		 */
		bool StartsWith(const TChar* prefix, bool ignoreCase = false) const
		{
			StringWrapper str = GetCStr();
			return str.StartsWith(prefix);
		}

		/**
		 * \brief Returns whether this string starts with given a char (Prefix).
		 */
		bool StartsWith(TChar c, bool ignoreCase = false) const
		{
			StringWrapper str = GetCStr();
			return str.StartsWith(c);
		}

		/**
		 * \brief Compares this string with given one.
		 */
		bool Equals(ConstString other, bool ignoreCase = false)
		{
			StringWrapper str = GetCStr();
			return str.Equals(other, ignoreCase);
		}

		bool operator==(const TChar* other) const { return Equals(GetCStr(), other); }
		bool operator==(const atBaseString& other) const { return Equals(GetCStr(), other); }

		atBaseString operator+(const TChar* other)
		{
			TSize length = GetLength() + StringLength(other);

			atBaseString result(length);
			result += *this;
			result += other;

			return result;
		}

		atBaseString operator+(const atBaseString& other)
		{
			TSize length = GetLength() + other.GetLength();

			atBaseString result(length);
			result += *this;
			result += other;

			return result;
		}

		atBaseString& operator+= (const TChar c)
		{
			Append(&c, 1);
			return *this;
		}

		atBaseString& operator+=(const TChar* other)
		{
			Append(other, String::Length(other));
			return *this;
		}

		atBaseString& operator+=(const atBaseString& other)
		{
			Append(other.GetCStr(), other.GetLength());
			return *this;
		}

		atBaseString& operator=(const TChar* other)
		{
			Set(other);
			return *this;
		}

		atBaseString& operator=(const atBaseString& other)
		{
			Set(other.GetCStr(), other.GetLength());
			return *this;
		}

		atBaseString& operator=(atBaseString&& other) noexcept
		{
			delete[] Buffer::m_Items;

			Buffer::m_Items = other.m_Items;
			Buffer::m_Size = other.m_Size;
			Buffer::m_Capacity = other.m_Capacity;

			other.m_Items = nullptr;

			return *this;
		}

		operator const TChar* () { return GetCStr(); }
		operator const TChar* () const { return GetCStr(); }

		TChar& operator[](s32 index) { return Buffer::operator[](index); }

		TChar* begin() { return Buffer::m_Items; }
		TChar* end() { return Buffer::m_Items + GetLength(); }
	};

	/**
	 * \brief ASCII string with maximum length of 65 535 characters.
	 */
	using atString = atBaseString<char>;

	/**
	 * \brief UTF16 string with maximum length of 65 535 characters.
	 */
	using atWideString = atBaseString<wchar_t>;
}
