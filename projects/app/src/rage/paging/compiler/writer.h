//
// File: writer.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "packer.h"
#include "common/types.h"
#include "am/system/ptr.h"
#include "rage/paging/resourceheader.h"
#include "rage/zlib/stream.h"

namespace rage
{
	class pgSnapshotAllocator;

	/**
	 * \brief Contains resource version and packed chunks.
	 */
	struct datCompileData
	{
		u32 Version;

		datPackedChunks VirtualChunks;
		datPackedChunks PhysicalChunks;

		datResourceHeader GetHeader() const;
	};

	/**
	 * \brief Writes and compresses resource from compiler streams.
	 */
	class pgRscWriter
	{
		typedef amPtr<char[]> TBuffer;

		// Size of purely resource structures and data.
		u32 m_RawSize;
		// Size of resource data fitted on large memory pages, hence size of allocated resource.
		u32 m_AllocSize;
		// Size of compressed resource file on disk.
		u32 m_FileSize;

		HANDLE m_File;

		zLibCompressor m_Compressor = { 30 * 1024u * 1024u }; // 30MB~ buffer

		const datCompileData* m_WriteData;
		const wchar_t* m_Path;

		u32 ComputeUsedSize(const datPackedChunks& packedPage) const;
		void WriteHeader() const;
		void WriteData(const datPackedChunks& packedPage, const pgSnapshotAllocator* pAllocator);
		void CompressAndWrite(pVoid data, u32 dataSize);

		bool OpenResource();
		void CloseResource() const;

		void ResetWriteStats();
		void PrintWriteStats() const;
	public:
		pgRscWriter() = default;

		bool Write(const wchar_t* path, const datCompileData& writeData);
	};
}
