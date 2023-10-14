//
// File: splitter.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <iterator>

#include "common/types.h"

namespace rageam
{
	/**
	 * \brief Splits a string on tokens.
	 * \tparam TArgs Split characters, for example - <' ', '_'>
	 */
	template<char... TArgs>
	class StringSplitter
	{
		static constexpr u32 STACK_BUFFER_SIZE = 256;

		char		m_SmallBuffer[STACK_BUFFER_SIZE] = {};
		char*		m_LargeBuffer = nullptr;
		bool		m_UseLargeBuffer;
		size_t		m_Offset = 0;
		size_t		m_Length;
		ConstString m_String;
		bool		m_TrimSpaces = false;

		bool TrimBuffer()
		{
			char* buffer = GetBuffer();
			if (buffer[0] == '\0') // Empty string, cull it out
				return false;

			// Index of character next to the last continuous whitespace from the
			size_t left = 0;  // beginning
			size_t right = 0; // end

			size_t offset = 0;
			while (buffer[offset] != '\0')
			{
				char c = buffer[offset];
				if (right == 0 && c == ' ')
					left = offset;

				if (c != ' ')
					right = offset;

				offset++;
			}

			if (left > right) // All whitespace
				return false;

			memcpy(buffer, buffer + left, right - left);
			return true;
		}

		char* GetBuffer() { return m_UseLargeBuffer ? m_LargeBuffer : m_SmallBuffer; }
	public:
		StringSplitter(ConstString begin, ConstString end = nullptr)
		{
			if (end)	m_Length = std::distance(begin, end);
			else		m_Length = strlen(begin);

			size_t size = m_Length + 1; // Including null terminator

			m_String = begin;
			m_UseLargeBuffer = size >= STACK_BUFFER_SIZE;
			if (m_UseLargeBuffer)
			{
				// We must allocate buffer long enough to fit
				// whole string in case if there's nothing to split
				m_LargeBuffer = new char[size];
			}
		}

		~StringSplitter()
		{
			delete[] m_LargeBuffer;
			m_LargeBuffer = nullptr;
		}

		void SetTrimSpaces(bool trim) { m_TrimSpaces = trim; }

		bool GetNext(ConstString& token, size_t* outOffset = nullptr, size_t* outLength = nullptr)
		{
			static constexpr char splitters[] = { TArgs... };

			char* buffer = GetBuffer();
			buffer[0] = '\0'; // Clear

			size_t tokenLength = 0;
			size_t tokenOffset = m_Offset;

			auto doSplit = [&]() -> bool
				{
					m_Offset++; // Skip splitter char

					buffer[tokenLength] = '\0'; // Finalize token word

					if (m_TrimSpaces)
					{
						if (TrimBuffer())
						{
							// Recompute length after trimming
							tokenLength = strlen(buffer);
						}
						else // Buffer was empty
						{
							// Search for next non empty token
							return GetNext(token);
						}
					}

					if (outOffset) *outOffset = tokenOffset;
					if (outLength) *outLength = tokenLength;
					token = buffer;

					return true;
				};

			while (m_Offset < m_Length)
			{
				char c = m_String[m_Offset];
				buffer[tokenLength] = c;

				// Try to split string...
				for (char splitter : splitters)
				{
					// Match found, split token
					if (c == splitter)
					{
						return doSplit();
					}
				}

				// No match found, keep building token...
				m_Offset++;
				tokenLength++;
			}

			// There was nothing to split or last token
			if (buffer[0] != '\0')
				return doSplit();

			return false;
		}
	};
}
