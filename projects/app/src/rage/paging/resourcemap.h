#pragma once

#include "paging.h"
#include "resourcechunk.h"
#include "common/types.h"

namespace rage
{
	class pgBase;

	/**
	 * \brief Holds chunks from resource file.
	 */
	struct datResourceMap
	{
		u8 VirtualChunkCount;
		u8 PhysicalChunkCount;

		u8 MainChunkIndex;						// In game resources first one is always the main one
		pgBase* MainChunk;							// Root class of the resource
		datResourceChunk Chunks[PG_MAX_CHUNKS];		// Virtual & Physical chunks combined

		u64 UnusedC10;

		// Gets total number of chunks (virtual + physical)
		u8 GetChunkCount() const { return VirtualChunkCount + PhysicalChunkCount; }

		datResourceChunk* begin() { return Chunks; }
		datResourceChunk* end() { return Chunks + GetChunkCount(); }
	};
}
