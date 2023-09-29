#include "builder.h"

#include "rage/paging/resourceheader.h"
#include "rage/system/systemheap.h"
#include "rage/zlib/stream.h"

bool rage::pgRscBuilder::ReadAndDecompressChunks(datResourceMap& map, fiDevice* device, ConstString path)
{
	u64 offset;
	fiHandle_t file = device->OpenBulk(path, offset);
	if (file == FI_INVALID_HANDLE)
	{
		AM_ERRF("pgRscBuilder::ReadAndDecompressChunks() -> Unable to open file for reading...");
		return false;
	}
	offset += sizeof datResourceHeader;

	char* fileBuffer = new char[READ_BUFFER_SIZE];

	AM_DEBUGF("pgRscBuilder::ReadAndDecompressChunks() -> Processing %u chunks (Virtual: %u, Physical: %u)", 
		map.GetChunkCount(), map.VirtualChunkCount, map.PhysicalChunkCount);

	u32 remaining = 0; // Remaining size indicate us that we need to read more data from file
	zLibDecompressor decompressor;
	for (u32 i = 0; i < map.GetChunkCount(); i++)
	{
		datResourceChunk& chunk = map.Chunks[i];

		pVoid	chunkDest = chunk.GetAllocatedAddress();
		u32		chunkSize = static_cast<u32>(chunk.Size);
		bool	done = false;
		while (!done)
		{
			u32 sizeReaded = 0;
			// We read from file & decompress until chunk is done, then move to next chunk
			if (remaining == 0)
			{
				sizeReaded = device->ReadBulk(file, offset, fileBuffer, READ_BUFFER_SIZE);
				if (sizeReaded == FI_INVALID_RESULT)
				{
					AM_ERRF("pgRscBuilder::ReadAndDecompressChunks() -> Failed to read file...");
					return false;
				}
				offset += sizeReaded;
			}

			done = decompressor.Decompress(
				chunkDest, chunkSize, fileBuffer, sizeReaded, remaining);
		}
	}
	device->Close(file);
	delete[] fileBuffer;
	return true;
}

bool rage::pgRscBuilder::PerformReadInMainThread(ConstString path, u32 version, datResourceMap& map, datResourceInfo& info)
{
	fiDevice* device = fiDevice::GetDeviceImpl(path);
	if (device->GetResourceInfo(path, info) != version)
	{
		AM_ERRF("pgRscBuilder::PerformReadInMainThread() -> File is not a valid resource, unable to read header.");
		return false;
	}

	info.GenerateMap(map);

	if (!AllocateMap(map))
	{
		AM_ERRF("pgRscBuilder::PerformReadInMainThread() -> Failed to allocate map.");
		return false;
	}

	if (!ReadAndDecompressChunks(map, device, path))
		return false;

	map.MainChunk = static_cast<pgBase*>(map.Chunks[map.MainChunkIndex].GetAllocatedAddress());

	return true;
}

void rage::pgRscBuilder::ConstructName(char* buffer, u32 bufferSize, const char* path)
{
	// This is currently placeholder because we can read only from fiLocalDevice,
	// additionally native implementation supports platform-independent file extension but we don't need that.
	sprintf_s(buffer, bufferSize, "%s", path);
}

rage::pgBase* rage::pgRscBuilder::LoadBuild(ConstString path, u32 version, datResourceMap& map, datResourceInfo& info)
{
	char fullName[256];
	ConstructName(fullName, 256, path);

	if (!PerformReadInMainThread(fullName, version, map, info))
		return nullptr;

	return map.MainChunk;
}

void rage::pgRscBuilder::Cleanup(const datResourceMap& map)
{
	sysMemAllocator* allocator = GetMultiAllocator()->GetAllocator(ALLOC_TYPE_PHYSICAL);

	// All physical data is uploaded to GPU, now we can free it
	for (u32 i = map.VirtualChunkCount - 1; i < map.GetChunkCount(); i++)
		allocator->Free(reinterpret_cast<pVoid>(map.Chunks[i].DestAddr));

	AM_DEBUGF("pgRscBuilder::Cleanup() -> De-allocated %u physical chunks.", map.PhysicalChunkCount);
}

bool rage::pgRscBuilder::AllocateMap(datResourceMap& map)
{
	sysMemAllocator* allocator = GetMultiAllocator();

	for (u32 i = 0; i < map.GetChunkCount(); i++)
	{
		datResourceChunk& chunk = map.Chunks[i];

		pVoid block;
		if (i < map.VirtualChunkCount) // Align doesn't matter here, buddy allocator doesn't support it.
			block = allocator->Allocate(chunk.Size, 16, ALLOC_TYPE_VIRTUAL);
		else
			block = allocator->Allocate(chunk.Size, 16, ALLOC_TYPE_PHYSICAL);

		// Failed to allocate one of the chunks, we'll have to free all previously allocated ones...
		if (!block)
		{
			for (u32 j = 0; j <= i; j++)
				allocator->Free(reinterpret_cast<pVoid>(map.Chunks[j].DestAddr));
			return false;
		}

		chunk.DestAddr = reinterpret_cast<u64>(block);
	}
	return true;
}
