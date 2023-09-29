//
// File: string.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "helpers/resharper.h"

// Defines size of thread-local temporary buffer in ToWideTemp/ToAnsiTemp conversion functions.
static constexpr u32 STRING_TEMP_BUFFER_SIZE = 2048;

/**
 * \brief Utility functions for ANSI and WIDE strings.
 */
class String
{
	static u32 GetCopySize(u32 sourceLength, int hintLength)
	{
		if (hintLength == -1)
			return sourceLength + 1;
		return (u32)hintLength < sourceLength ? (u32)hintLength + 1 : sourceLength + 1; // Including null-terminator.
	}
public:
	/**
	 * \brief Gets empty string with given character type.
	 */
	template<typename TChar>
	static const TChar* Empty()
	{
		static constexpr TChar empty = TChar(0);
		return &empty;
	}

	static bool IsNullOrEmpty(const char* string);
	static bool IsNullOrEmpty(const wchar_t* string);

	/**
	 * \brief Calculates number of ANSI characters until null-terminator character.
	 */
	static u32 Length(const char* string);

	/**
	 * \brief Calculates number of UTF16 characters until null-terminator.
	 */
	static u32 Length(const wchar_t* string);

	/**
	 * \brief Formats ANSI string using thread-local buffer.
	 * Buffer size is defined by STRING_TEMP_BUFFER_SIZE and must be long enough for any regular operation.
	 * \return Temporary formatted string, see remarks.
	 * \remarks Returned string is valid until next call of this function in current thread.
	 */
	PRINTF_ATTR(1, 2) static const char* FormatTemp(const char* fmt, ...);

	/**
	 * \brief Formats UTF16 string using thread-local buffer.
	 * Buffer size is defined by STRING_TEMP_BUFFER_SIZE and must be long enough for any regular operation.
	 * \return Temporary formatted string, see remarks.
	 * \remarks Returned string is valid until next call of this function in current thread.
	 */
	WPRINTF_ATTR(1, 2) static const wchar_t* FormatTemp(const wchar_t* fmt, ...);

	/**
	 * \brief Converts UTF16 Wide string to UTF8 Multi-Byte string.
	 * \param destination		String buffer where converted string will be copied to.
	 * \param destinationSize	Size of string buffer.
	 * \param source			String to convert.
	 * \remarks Note that in UTF8 characters have different sizes,
	 *			meaning that buffer size does no reflect actual symbol capacity.
	 */
	static void WideToUtf8(char* destination, int destinationSize, const wchar_t* source);

	/**
	 * \brief Converts UTF8 string to UTF16 string.
	 * \param destination		String buffer where converted string will be copied to.
	 * \param destinationSize	Size of string buffer.
	 * \param source			String to convert.
	 */
	static void Utf8ToWide(wchar_t* destination, int destinationSize, const char* source);

	/**
	 * \brief Copies characters from source string until null-terminator is reached.
	 * \param destination		String buffer where string will be copied to.
	 * \param destinationSize	Size of string buffer,
	 *							source string length (including null-terminator) must be less or equal to it.
	 * \param source			String to copy characters from.
	 * \param length			Length of the source string to copy.
	 */
	static void Copy(char* destination, int destinationSize, const char* source, int length = -1);

	/**
	 * \brief Copies characters from source string until null-terminator is reached.
	 * \param destination		String buffer where string will be copied to.
	 * \param destinationSize	Size of string buffer,
	 *							source string length (including null-terminator) must be less or equal to it.
	 * \param source			String to copy characters from.
	 * \param length			Length of the source string to copy.
	 */
	static void Copy(wchar_t* destination, int destinationSize, const wchar_t* source, int length = -1);

	/**
	 * \brief Converts ansi (extended ASCII) string to wide (UTF16) string.
	 * \param destination		String buffer where converted string will be written to.
	 * \param destinationSize	Size of string buffer,
	 *							it must be greater or equal to source string size (length including null-terminator).
	 * \param source			String to convert.
	 */
	static void ToWide(wchar_t* destination, int destinationSize, const char* source);

	/**
	 * \brief Converts wide (UTF16) string to ansi (extended ASCII) string.
	 * \param destination		String buffer where converted string will be written to.
	 * \param destinationSize	Size of string buffer,
	 *							it must be greater or equal to source string size (length including null-terminator).
	 * \param source			String to convert.
	 * \remarks ASCII char set is smaller than UTF16, any non supported character will be replaced to whitespace (' ').
	 */
	static void ToAnsi(char* destination, int destinationSize, const wchar_t* source);

	/**
	 * \brief Converts wide (UTF16) to UTF8 string using thread-local buffer.
	 * Buffer size is defined by STRING_TEMP_BUFFER_SIZE and must be long enough for any regular operation.
	 * \param source	String to convert.
	 * \return			Temporary converted string, see remarks.
	 * \remarks Returned string is valid until next call of this function in current thread.
	 */
	static const char* ToUtf8Temp(const wchar_t* source);

	/**
	 * \brief Converts ansi (extended ASCII) to wide (UTF16) string using thread-local buffer.
	 * Buffer size is defined by STRING_TEMP_BUFFER_SIZE and must be long enough for any regular operation.
	 * \param source	String to convert.
	 * \return			Temporary converted string, see remarks.
	 * \remarks Returned string is valid until next call of this function in current thread.
	 */
	static const wchar_t* ToWideTemp(const char* source);

	/**
	 * \brief Converts wide (UTF16) string to ansi (extended ASCII) string using thread-local buffer.
	 * Buffer size is defined by STRING_TEMP_BUFFER_SIZE and must be long enough for any regular operation.
	 * \param source	String to convert.
	 * \return			Temporary converted string, see remarks.
	 * \remarks Returned string is valid until next call of this function in current thread.
	 */
	static const char* ToAnsiTemp(const wchar_t* source);

	/**
	 * \brief Compares two given ANSI strings.
	 */
	static bool Equals(ConstString left, ConstString right, bool ignoreCase = false);

	/**
	 * \brief Compares two given UTF16 strings.
	 */
	static bool Equals(ConstWString left, ConstWString right, bool ignoreCase = false);

	/**
	 * \brief Formats ANSI string.
	 * \param destination		String buffer where converted string will be written to.
	 * \param destinationSize	Size of string buffer.
	 * \param fmt				C Format string.
	 * \param args				Variadic arguments.
	 */
	static void FormatVA(char* destination, int destinationSize, const char* fmt, va_list args);

	/**
	 * \brief Formats UTF16 string.
	 * \param destination		String buffer where converted string will be written to.
	 * \param destinationSize	Size of string buffer.
	 * \param fmt				C Format string.
	 * \param args				Variadic arguments.
	 */
	static void FormatVA(wchar_t* destination, int destinationSize, const wchar_t* fmt, va_list args);
};
