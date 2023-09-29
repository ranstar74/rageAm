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
	// TODO: Path comparison doesn't work properly with '/', '\\'

#define PATH_SEPARATOR '\\'

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

		/**
		 * \brief Joins current path with given token using '/' separator symbol if needed.
		 */
		void Join(TCString token)
		{
			TString cursor = m_Buffer;

			u32 length = StringWrapper(m_Buffer).Length();
			s32 avail = TSize - length;
			cursor += length;

			if (!Char::IsPathSeparator(cursor[0]))
			{
				cursor[0] = PATH_SEPARATOR;

				++cursor;
				--avail;
			}
			AM_ASSERT(avail >= 0, "PathBase::Append() -> Out of memory!");

			String::Copy(cursor, avail, token);
		}

		/**
		 * \brief Appends given token to the end of current path as just text.
		 */
		void Append(TCString token)
		{
			TString cursor = m_Buffer;

			u32 length = StringWrapper(m_Buffer).Length();
			cursor += length;

			String::Copy(cursor, MAX_PATH - length, token);
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
		 * \brief Performs C string format into internal buffer.
		 */
		void Format(TString fmt, ...)
		{
			va_list args;
			va_start(args, fmt);
			String::FormatVA(m_Buffer, TSize, fmt, args);
			va_end(args);
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
