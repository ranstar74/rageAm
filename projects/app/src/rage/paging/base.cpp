#include "base.h"

#include "resource.h"
#include "resourcemap.h"
#include "resourcechunk.h"
#include "rage/system/systemheap.h"
#include "compiler/compiler.h"
#include "compiler/snapshotallocator.h"
#include "am/system/asserts.h"
#include "common/logger.h"

bool rage::pgBase::Map::IsCompiled() const
{
	return !bIsDynamic;
}

void rage::pgBase::Map::GenerateFromMap(const datResourceMap& map)
{
	VirtualChunkCount = map.VirtualChunkCount;
	PhysicalChunkCount = map.PhysicalChunkCount;
	MainChunkIndex = map.MainChunkIndex;

	bIsDynamic = false;

	for (u8 i = 0; i < map.GetChunkCount(); i++)
	{
		const datResourceChunk& chunk = map.Chunks[i];

		// NOTE: Destination address has to be aligned to 16 for this to work!
		//  If value is multiple of 16, lowest 8 bits are always all null.
		AddressAndShift[i] = chunk.DestAddr | chunk.GetSizeShift();
	}
}

void rage::pgBase::Map::RegenerateMap(datResourceMap& map) const
{
	map.PhysicalChunkCount = 0;
	map.VirtualChunkCount = VirtualChunkCount;

	for (u8 i = 0; i < VirtualChunkCount; i++)
	{
		u64 address = AddressAndShift[i] & MAP_ADDRESS_MASK;
		u8 shift = AddressAndShift[i] & MAP_SIZE_SHIFT_MASK;

		map.Chunks[i] = datResourceChunk(address, address, PG_MIN_CHUNK_SIZE << shift);
	}

	map.MainChunkIndex = MainChunkIndex;
	map.MainChunk = reinterpret_cast<pgBase*>(map.Chunks[MainChunkIndex].DestAddr);
}

void rage::pgBase::MakeDefragmentable(const datResourceMap& map) const
{
	if (!HasMap())
		return;

	m_Map->GenerateFromMap(map);
}

void rage::pgBase::FreeMemory(const datResourceMap& map) const
{
	if (m_Map->bIsDynamic)
		return;

	// Important thing to note here is that sysMemGrowBuddyAllocator allows 'invalid'
	// or already deleted pointers, it is really weird but for paged resources
	// first destructor is called, where it will free up all the things and then
	// this function, which will do basically nothing because everything been already cleaned up

	sysMemAllocator* allocator = GetMultiAllocator()->GetAllocator(ALLOC_TYPE_VIRTUAL);
	for (u8 i = 0; i < map.VirtualChunkCount; i++)
		allocator->Free(reinterpret_cast<void*>(map.Chunks[i].DestAddr));

	AM_DEBUGF("pgBase::FreeMemory() -> Deallocated %u virtual chunk(s).", map.VirtualChunkCount);
}

rage::pgBase::pgBase()
{
	datResource* rsc = datResource::GetCurrent();

	// ReSharper disable once CppObjectMemberMightNotBeInitialized
	if (rsc && m_Map) // Only root pgBase has map
	{
		rsc->Fixup(m_Map);
		MakeDefragmentable(*rsc->Map);
		return;
	}
	m_Map = nullptr;
}

rage::pgBase::~pgBase()
{
	Destroy();
}

rage::pgBase::pgBase(const pgBase& other)
{
	// Just like in atArray, we have to allocate memory for
	// mini map (which presents only in compiled / file resources) manually.
	pgSnapshotAllocator* pAllocator = pgRscCompiler::GetVirtualAllocator();
	if (!pAllocator || !pgRscCompiler::IsRootResourceAllocation(this))
	{
		// We're not compiling resource so no need to have a map.
		m_Map = nullptr;
		return;
	}
	pAllocator->AllocateRef(m_Map);
}

void rage::pgBase::Destroy() const
{
	if (!HasMap()) // Not compiled resource
	{
		sysMemAllocator* allocator = GetMultiAllocator()->GetAllocator(ALLOC_TYPE_VIRTUAL);
		if (allocator)
		{
			//AM_DEBUGF("pgBase::Destroy() -> Not compiled resource, but allocated on heap, deleting this.");
			allocator->Free((pVoid)this);
		}
		else
		{
			//AM_DEBUGF("pgBase::Destroy() -> Not compiled resource nor allocated on heap.");
		}
		return;
	}
	//AM_DEBUGF("pgBase::Destroy() -> Deallocating resource map.");

	datResourceMap map;
	RegenerateMap(map);
	FreeMemory(map);

	// Nothing must be done after de-allocating memory!
	// Consider FreeMemory as delete this;
}

bool rage::pgBase::HasMap() const
{
	if (!m_Map)
		return false;
	return m_Map->IsCompiled();
}

void rage::pgBase::RegenerateMap(datResourceMap& map) const
{
	if (!HasMap())
		return;

	m_Map->RegenerateMap(map);
}
