//
// File: bbuffer.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/types.h"
#include "am/file/fileutils.h"
#include "helpers/fourcc.h"
#include "fileutils.h"

#include <zlib-ng.h>

namespace rageam::file
{
	/**
	 * \brief Binary Read/Write stream with functionality to write binaries.
	 */
	class BinaryBuffer
	{
		static constexpr u32 COMPRESSED_MAGIC   = FOURCC('Z', 'B', 'U', 'F');
		static constexpr u32 UNCOMPRESSED_MAGIC = FOURCC(' ', 'B', 'U', 'F');

		List<char>   m_Bytes;
		int		     m_ReadCursor = 0;
		ConstWString m_CurrentPath = L"";

		// Attempts to write to stream
		// Writes error message to console if fails and automatically closes handle
		bool WriteStream(const void* buffer, size_t size, FILE*& file) const
		{
			if (!WriteFileStream(buffer, size, file))
			{
				AM_TRACEF(L"BinaryBuffer -> Failed to write to file '%ls'.", m_CurrentPath);
				CloseFileStream(file);
				file = nullptr;
				return false;
			}
			return true;
		}

		// Attempts to read stream
		// Writes error message to console if fails and automatically closes handle
		bool ReadStream(void* out, size_t size, FILE*& file) const
		{
			if (ReadFileStream(out, size, size, file) != size)
			{
				AM_TRACEF(L"BinaryBuffer -> Failed to read file '%ls'.", m_CurrentPath);
				CloseFileStream(file);
				file = nullptr;
				return false;
			}
			return true;
		}

	public:
		template<typename T>
		void Write(const T& value)
		{
			m_Bytes.GrowCapacity(m_Bytes.GetSize() + sizeof(T));
			for (size_t i = 0; i < sizeof(T); i++) 
				m_Bytes.Add({});
			memcpy(m_Bytes.GetItems() + m_Bytes.GetSize() - sizeof(T), &value, sizeof(T));
		}

		template<typename T>
		T Read()
		{
			AM_ASSERT(m_ReadCursor < m_Bytes.GetSize(), "BinaryBuffer::Read() -> End of stream!");
			T value = *reinterpret_cast<T*>(m_Bytes.GetItems() + m_ReadCursor);
			m_ReadCursor += sizeof(T);
			return value;
		}

		bool ToFile(ConstWString path, bool compress = true)
		{
			EASY_FUNCTION();
			m_CurrentPath = path;
			FILE* file = OpenFileStream(path, L"wb");
			if (!file)
			{
				AM_TRACEF(L"BinaryBuffer::ToFile() -> Failed to open file '%ls'.", path);
				return false;
			}
			// Write magic to indicate whether file is compressed
			if (!WriteStream(compress ? &COMPRESSED_MAGIC : &UNCOMPRESSED_MAGIC, 4, file))
				return false;
			if (compress)
			{
				// Compressed data will be at least as big as source buffer
				auto compressedBuffer = std::unique_ptr<char[]>(new char[m_Bytes.GetSize()]);
				size_t compressedLenth = m_Bytes.GetSize();
				int zresult = zng_compress((uint8_t*)compressedBuffer.get(), &compressedLenth, (uint8_t*)m_Bytes.GetItems(), m_Bytes.GetSize());
				if (zresult < 0)
				{
					AM_TRACEF(L"BinaryBuffer::ToFile() -> Compressor failed with code '%i', path: %ls.", zresult, path);
					CloseFileStream(file);
					return false;
				}
				AM_ASSERTS(compressedLenth < 0xFFFFFFFF); // More than 4GB? Most likely something went wrong...
				// Write length and compressed stream
				if (!WriteStream(&compressedLenth, 4, file))
					return false;
				if (!WriteStream(compressedBuffer.get(), compressedLenth, file))
					return false;
			}
			else
			{
				u32 size = m_Bytes.GetSize();
				if (!WriteStream(&size, 4, file))
					return false;
				// Write length and uncompressed stream
				if (!WriteStream(m_Bytes.GetItems(), m_Bytes.GetSize(), file))
					return false;
			}
			CloseFileStream(file);
			return true;
		}

		bool FromFile(ConstWString path)
		{
			EASY_FUNCTION();
			m_CurrentPath = path;
			FILE* file = OpenFileStream(path, L"wb");
			if (!file)
			{
				AM_TRACEF(L"BinaryBuffer::FromFile() -> Failed to open file '%ls'.", path);
				return false;
			}
			// First 8 bytes are magic and size
			u32 magic;
			u32 size;
			if (!ReadStream(&magic, 4, file) || !ReadStream(&size, 4, file))
				return false;
			if (magic != COMPRESSED_MAGIC && magic != UNCOMPRESSED_MAGIC)
			{
				AM_TRACEF(L"BinaryBuffer::FromFile() -> Invalid magic '%X', file: '%ls'.", magic, path);
				CloseFileStream(file);
				return false;
			}
			// Process compressed stream
			if (magic == COMPRESSED_MAGIC)
			{
				auto compressedBuffer = std::unique_ptr<char[]>(new char[size]);
				u32  uncompressedSize;
				if (!ReadStream(&uncompressedSize, 4, file))
					return false;
				if (!ReadStream(compressedBuffer.get(), size, file))
					return false;
				m_Bytes.Reserve(uncompressedSize, true);
				size_t bytesLen = uncompressedSize;
				zng_uncompress((uint8_t*)m_Bytes.GetItems(), &bytesLen, (uint8_t*)compressedBuffer.get(), size);
			}
			else // Process raw stream
			{
				m_Bytes.Reserve(size, true);
				if (!ReadStream(m_Bytes.GetItems(), size, file))
					return false;
			}
			CloseFileStream(file);
			return true;
		}
	};
}
