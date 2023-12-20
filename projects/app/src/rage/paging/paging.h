#pragma once

#include "common/types.h"

/*
 * Before exploring code below, it may be useful to understand page memory management in operating systems.
 * https://stackoverflow.com/questions/14347206/what-are-the-differences-between-virtual-memory-and-physical-memory
 *
 * There's good russian post on this system: https://sannybuilder.com/forums/viewtopic.php?id=433&p=1
 * Note that names in this post are not entirely accurate (because we got them only in 2018 from Companion RDR2 android app).
 *
 * Resources are files with extensions #ft, #dr, #td, #cd and so on.
 * Short-term: Resources are memory dumps. Loading resource is done via copying memory dumps back in memory and fixing pointers.
 *
 * Compilation process in 3 main steps: (see pgRscCompiler for more details)
 *  - Copying structure hierarchy (done via pgRscSnapshotAllocator)
 *	- Packing individual structures into chunks with sizes of power-of-two (pgRscPacker)
 *  - Writing & compressing packed together chunks to file (pgRscWriter)
 *
 * All resource addresses are split in two types: virtual and physical (in CodeWalker / Open IV terminology they're called system and graphical),
 * The only difference is that physical addresses point to data that will be allocated GPU (for i.e. texture pixel data)
 * and de-allocated right after resource is constructed (see pgRscBuilder::CleanUp)
 *
 * Building resource is done in following steps:
 * - Read resource header (datResourceInfo)
 * - Build map from encoded data (datResourceInfo, datResourceMap)
 * - Read resource data from file
 * - Allocate chunks in process memory at chunk destination address with given chunk size
 * - Iterate through each chunk in datResourceMap, decompress with zlib and copy to allocated chunk memory (destination address)
 * (NOTE: you know size to decompress from chunk size, and this is really helpful)
 * - Now you have resources in your memory, invoke placement new constructor on that memory for resource structure
 * (Resource structure (for e.g. pgDictionary<grcTexture>) is identified only by file extension).
 * - Placement new constructor (see pgDictionary(const datResource&)) adjusts field offsets using Fixup function
 * (which just look ups for chunk and takes it's destination address)
 * - De-allocate physical chunks (GPU)
 * - Done! Resource is ready to be used.
 */

namespace rage
{
	// Idea to 'categorize' addresses by highest bit comes from ancient times
	static constexpr u64 PG_VIRTUAL_MASK = 0x50000000; // RAM
	static constexpr u64 PG_PHYSICAL_MASK = 0x60000000; // GPU

	static constexpr u32 PG_MIN_CHUNK_SIZE = 0x2000; // Smallest block size in sysMemBuddyAllocator

	// Not sure what's exact purpose of shifting possible range from (2KB - 268MB) to (131KB - 4294MB)
	// Could it be a mistake? Because if we interpret 0x2000 as 2000, it gives range from 20KB to 100MB
	// and 100MB is maximum allowed chunk size by pgReader
	// NEW: grcTexture stores physical size with mask 0x7FFFF80, highest bit is 0x400'0000 too,
	// it seems to be used for debug metrics and for searching low-resolution images in CDTV class
	// Another NOTE: This limit seems to be as simple as sane check.
	static constexpr u32 PG_MIN_SIZE_SHIFT = 4;

	static constexpr u32 PG_MAX_CHUNKS = 128;

	// Buckets are ordered group of chunks, descending by size,
	// defined by how chunks are encoded (see paging/datResourceInfo.h)
	static constexpr u32 PG_MAX_BUCKETS = 9;

	// Size of lowest possible chunk at bucket 0
	static constexpr u32 PG_MIN_BUCKET_CHUNK_SIZE = 0x2000 << PG_MIN_SIZE_SHIFT;

	static constexpr u32 PG_MIN_CHUNK_SIZE_SHIFT = 13; // 1 << 13 = 0x2000

	// Actual limit of pgStreamer is 100MB, this is closest chunk size to it (134MB)
	static constexpr u32 PG_MAX_CHUNK_SIZE = 0x8000000;
}
