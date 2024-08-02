//
// File: path.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <Windows.h>

#include "am/string/char.h"
#include "am/string/string.h"
#include "am/string/stringwrapper.h"
#include "am/system/asserts.h"
#include "pathutils.h"
#include "common/types.h"

namespace rageam::file
{
	/**
	 * \brief Utility that helps constructing file system paths.
	 * \remarks Internal buffer is stack-allocated array of size defined by MAX_PATH macro.
	 */
	template<typename TChar, u32 TSize = MAX_PATH>
	class PathBase
	{
		using TCString = const TChar*;
		using TString = TChar*;
		using Char = CharBase<TChar>;

		TChar m_Buffer[TSize] = {};
	public:
		PathBase(const TCString path)
		{
			String::Copy(m_Buffer, TSize, path);
		}
		PathBase(const PathBase& other) : PathBase(other.m_Buffer) {}
		PathBase() = default;

		bool IsEmpty() { return m_Buffer[0] == '\0'; }

		/**
		 * \brief Compares two path's, must be used instead of String::Equals because
		 * 'C:/Videos' is not the same string as 'C:\\Videos\\' but the same path.
		 * \remarks Paths 'C:/Videos/../' and 'C:/Videos/' are not considered equal by this function.
		 */
		bool Equals(TCString other) const
		{
			u32 cursor = 0;
			while (true)
			{
				char lhs = m_Buffer[cursor];
				char rhs = other[cursor];
				char lhsn = m_Buffer[cursor + 1];
				char rhsn = other[cursor + 1];

				cursor++;

				// End of paths, match
				if (lhs == '\0' && rhs == '\0')
					return true;

				// Trailing separator
				if (lhs == '\0' && Char::IsPathSeparator(rhs) && rhsn == '\0' ||
					rhs == '\0' && Char::IsPathSeparator(lhs) && lhsn == '\0')
				{
					return true;
				}

				// Both are separators
				if (Char::IsPathSeparator(lhs) && Char::IsPathSeparator(rhs))
				{
					continue;
				}
				
				if (lhs != rhs)
					return false;
			}
		}

		/**
		 * \brief Joins current path with given token using '/' separator symbol if needed.
		 */
		void Join(TCString token, int tokenLength = -1)
		{
			TString cursor = m_Buffer;

			u32 length = StringWrapper(m_Buffer).Length();
			s32 avail = TSize - length;

			if (length == 0)
			{
				Append(token, tokenLength);
				return;
			}

			if (length > 1) // Move cursor to last symbol
				cursor += length - 1;

			if (!Char::IsPathSeparator(cursor[0]))
			{
				++cursor;
				--avail;
				cursor[0] = Char::PathSeparator();
			}
			AM_ASSERT(avail >= 0, "PathBase::Append() -> Out of memory!");

			++cursor; // Move past separator

			String::Copy(cursor, avail, token, tokenLength);
		}

		/**
		 * \brief Appends given token to the end of current path as just text.
		 */
		void Append(TCString token, int tokenLength = -1)
		{
			TString cursor = m_Buffer;

			u32 length = StringWrapper(m_Buffer).Length();
			cursor += length;

			String::Copy(cursor, MAX_PATH - length, token, tokenLength);
		}

		/**
		 * \brief Gets extension from given path; Empty string if path don't have extension.
		 * \note "GrandTheftAutoV/GTA5.exe" -> "exe"
		 */
		PathBase GetExtension() const
		{
			return PathBase(file::GetExtension(m_Buffer));
		}

		/**
		 * \brief Gets file name including extension from given path.
		 * \note "GrandTheftAutoV/GTA5.exe" -> "GTA5.exe"
		 */
		PathBase GetFileName() const
		{
			return PathBase(file::GetFileName(m_Buffer));
		}

		/**
		 * \brief Gets file path without extension.
		 * \note "GrandTheftAutoV/GTA5.exe" -> "GrandTheftAutoV/GTA5"
		 */
		PathBase GetFilePathWithoutExtension() const
		{
			PathBase result;
			file::GetFilePathWithoutExtension(result.m_Buffer, TSize, m_Buffer);
			return result;
		}

		/**
		 * \brief Gets file name without extension from given path.
		 * \note "GrandTheftAutoV/GTA5.exe" -> "GTA5"
		 */
		PathBase GetFileNameWithoutExtension() const
		{
			PathBase result;
			file::GetFileNameWithoutExtension(result.m_Buffer, TSize, m_Buffer);
			return result;
		}

		/**
		 * \brief Gets parent directory (previous token in path).
		 * \note "src/am/file" -> "src/am"
		 */
		PathBase GetParentDirectory() const
		{
			PathBase result;
			file::GetParentDirectory(result.m_Buffer, TSize, m_Buffer);
			return result;
		}

		/**
		 * \brief Converts all separators to one type.
		 */
		PathBase Normalized() const
		{
			PathBase result;

			u32 i = 0;
			while (true)
			{
				Char c = m_Buffer[i];

				if (Char::IsPathSeparator(c))
				{
					result.m_Buffer[i] = Char::PathSeparator();
					// Path ends with separator... remove it
					if (!m_Buffer[i + 1])
					{
						result.m_Buffer[i + 1] = 0;
						break;
					}
				}
				else
				{
					result.m_Buffer[i] = c;
				}
				i++;

				if (c == '\0')
					break;
			}

			return result;
		}

		/**
		 * \brief Converts all separators to one type.
		 */
		void Normalize()
		{
			TChar* c = m_Buffer;
			while (*c)
			{
				if (Char::IsPathSeparator(*c))
				{
					*c = Char::PathSeparator();
					// Path ends with separator... remove it
					if (!c[1])
						c[1] = '\0';
				}
				++c;
			}
		}

		void ToLower()
		{
			StringWrapper(m_Buffer).ToLower();
		}
		
		/**
		 * \brief Performs C string format into internal buffer.
		 */
		void Format(TString fmt, ...)
		{
			va_list args;
			va_start(args, fmt);
			String::FormatVA(m_Buffer, TSize, fmt, args);
			va_end(args);
		}

		/**
		 * \brief This is a simple implementation that only cuts 'prefix'.
		 */
		PathBase GetRelativePath(TCString relativeTo)
		{
			s32 index = ImmutableString(m_Buffer).IndexOf(relativeTo, true);
			if (index == -1)
				return *this;
			PathBase result;
			TCString cursor = m_Buffer + index + ImmutableString(relativeTo).Length();
			if (Char::IsPathSeparator(cursor[0]))
				++cursor;
			String::Copy(result.m_Buffer, TSize, m_Buffer + index + ImmutableString(relativeTo).Length());
			return result;
		}

		u32 GetBufferSize() const { return TSize; }
		TString GetBuffer() { return m_Buffer; }
		TCString GetCStr() const { return m_Buffer; }

		/**
		 * \brief Places given token after current path, handling path separator.
		 */
		PathBase& operator/=(TCString token)
		{
			Join(token);
			return *this;
		}

		/**
		 * \brief Appends given token to the end of current path as just text.
		 */
		PathBase& operator+=(TCString token)
		{
			Append(token);
			return *this;
		}

		/**
		 * \brief Appends given token in the end of current path and returns copy.
		 */
		PathBase operator/(TCString token) const
		{
			PathBase result(m_Buffer);
			result.Join(token);
			return result;
		}

		/**
		 * \brief Appends given token to the end of current path as just text and returns copy.
		 */
		PathBase operator+(TCString token)
		{
			PathBase result(m_Buffer);
			result.Append(token);
			return result;
		}

		PathBase& operator=(TCString path)
		{
			String::Copy(m_Buffer, TSize, path);
			return *this;
		}

		PathBase& operator=(const PathBase& other) // NOLINT(bugprone-unhandled-self-assignment)
		{
			String::Copy(m_Buffer, TSize, other.m_Buffer);
			return *this;
		}

		bool operator==(const TCString other) const
		{
			return Equals(other);
		}

		bool operator==(const PathBase& other) const
		{
			return Equals(other);
		}

		operator TCString() const { return m_Buffer; }
	};

	// Ansi path utility, allocated on stack with PATH_MAX-sized buffer.
	using Path = PathBase<char>;

	// Wide path utility, allocated on stack with PATH_MAX-sized buffer.
	using WPath = PathBase<wchar_t>;

	// UTF8 path utility, allocated on stack with PATH_MAX-sized buffer.
	using U8Path = PathBase<char>;

	class PathConverter
	{
	public:
		/**
		 * \brief Converts UTF16 path to UTF8.
		 */
		static U8Path WideToUtf8(const WPath& widePath)
		{
			U8Path result;
			String::WideToUtf8(result.GetBuffer(), MAX_PATH, widePath);
			return result;
		}

		static WPath Utf8ToWide(const U8Path& utf8Path)
		{
			WPath result;
			String::Utf8ToWide(result.GetBuffer(), MAX_PATH, utf8Path);
			return result;
		}
	};

#define PATH_TO_UTF8(path) rageam::file::PathConverter::WideToUtf8(path)
#define PATH_TO_WIDE(path) rageam::file::PathConverter::Utf8ToWide(path)
}
