//
// File: stream.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "zlib.h"

#include "common/types.h"
#include "am/system/asserts.h"

static constexpr int ZLIB_WINDOW_BITS = -15;
static constexpr int ZLIB_COMPRESSION_LEVEL = 5;
static constexpr int ZLIB_MEMORY_LEVEL = 8;

class zLibCompressor
{
	static constexpr u32 COMPRESS_BUFFER_SIZE = 0x1000;

	using TBuffer = Bytef*;
	using TStream = z_stream;

	TBuffer m_Buffer;
	u32		m_BufferSize;
	bool	m_OwnBuffer;

	bool	m_Started = false;

	TStream	m_Stream{};

	void Init()
	{
		int status = deflateInit2(&m_Stream,
			ZLIB_COMPRESSION_LEVEL,
			Z_DEFLATED,
			ZLIB_WINDOW_BITS,
			ZLIB_MEMORY_LEVEL,
			Z_DEFAULT_STRATEGY);

		AM_ASSERT(status >= 0, "zLibDecompressor() -> Init failed with status %i", status);
	}
public:
	zLibCompressor(u32 bufferSize)
	{
		m_Buffer = new Bytef[bufferSize];
		m_BufferSize = bufferSize;
		m_OwnBuffer = true;

		Init();
	}

	zLibCompressor() : zLibCompressor(COMPRESS_BUFFER_SIZE) {}

	// Initializes compressor with user-specified temporary buffer where all compressed data will be written to.
	zLibCompressor(pVoid buffer, u32 bufferSize)
	{
		m_Buffer = static_cast<TBuffer>(buffer);
		m_BufferSize = bufferSize;
		m_OwnBuffer = false;

		Init();
	}

	~zLibCompressor()
	{
		if (m_OwnBuffer) delete[] m_Buffer;

		deflateEnd(&m_Stream);
	}

	/**
	 * \brief Compresses data.
	 *
	 * \param data				Buffer with data to compress.
	 * \param dataSize			Size of data buffer.
	 * \param compressedBuffer	Will be set to internal buffer with compressed data, contents of this buffer will be erased on next call.
	 * \param compressedSize	Size of compressed data in compressed buffer.
	 * \return True if all data was compressed, otherwise False.
	 */
	bool Compress(pVoid data, u32 dataSize, pVoid& compressedBuffer, u32& compressedSize)
	{
		if (!m_Started)
		{
			m_Stream.next_in = static_cast<TBuffer>(data);
			m_Stream.avail_in = dataSize;
			m_Started = true;
		}

		m_Stream.next_out = m_Buffer;
		m_Stream.avail_out = m_BufferSize;

		int status = deflate(&m_Stream, Z_SYNC_FLUSH);

		AM_ASSERT(status >= 0, "zLibCompressor::Compress() -> Failed with status %i", status);

		compressedBuffer = m_Buffer;
		compressedSize = m_BufferSize - m_Stream.avail_out;

		bool done = m_Stream.avail_in == 0;
		if (done) m_Started = false;
		return done;
	}

	void CompressAll(pVoid data, u32 dataSize, void writeFn(pVoid compressedBuffer, u32 compressedSize))
	{
		bool done = false;
		while (!done)
		{
			pVoid outBuffer;
			u32	outSize;

			done = Compress(data, dataSize, outBuffer, outSize);

			writeFn(outBuffer, outSize);
		}
	}
};

class zLibDecompressor
{
	using TBuffer = Bytef*;
	using TStream = z_stream;

	bool	m_Started = false;

	TStream	m_Stream{};
public:
	zLibDecompressor()
	{
		int status = inflateInit2(&m_Stream, ZLIB_WINDOW_BITS);
		AM_ASSERT(status >= 0, "zLibDecompressor() -> Init failed with status %i", status);
	}

	~zLibDecompressor()
	{
		inflateEnd(&m_Stream);
	}

	/**
	 * \brief Decompresses data.
	 *
	 * \param bufferOut			Buffer where decompressed data will be written to.
	 * \param bufferOutSize		Total size to decompress.
	 * \param bufferIn			Buffer with compressed data.
	 * \param bufferInSize		Size of buffer with compressed data.
	 * \param leftSize			How much of compressed buffer size is left after decompression,
	 *	if this value is 0 and compression is not finished, compressed data has to be renewed.
	 * \return True if data was fully decompressed, otherwise False.
	 */
	bool Decompress(pVoid bufferOut, u32 bufferOutSize, pVoid bufferIn, u32 bufferInSize, u32& leftSize)
	{
		if (!m_Started)
		{
			m_Stream.next_out = static_cast<TBuffer>(bufferOut);
			m_Stream.avail_out = bufferOutSize;
			m_Started = true;
		}

		if (m_Stream.avail_in == 0)
		{
			m_Stream.next_in = static_cast<TBuffer>(bufferIn);
			m_Stream.avail_in = bufferInSize;
		}

		int status = inflate(&m_Stream, Z_SYNC_FLUSH);

		AM_ASSERT(status >= 0, "zLibDecompressor::Decompress() -> Failed with status %i", status);

		leftSize = m_Stream.avail_in;

		bool done = m_Stream.avail_out == 0; // Decompressed all given buffer
		if (done) m_Started = false;
		return done;
	}
};
