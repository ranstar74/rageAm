#pragma once

#include "common/types.h"

namespace rage
{
	struct datResourceMap;

	/**
	 * \brief Base class for every paged resource.
	 * \n For more details on how this system works, see paging.h;
	 */
	class pgBase
	{
		/**
		 * \brief Mini copy of datResourceMap.
		 * \n Purpose: We have to store allocated chunks to de-allocate them when object is destroyed.
		 * \n Full datResourceMap contains redundant information for deallocation so this is a 'mini' version of it.
		 */
		struct Map
		{
			static constexpr u64 MAP_SIZE_SHIFT_MASK = 0xF;	// Lowest 8 bits
			static constexpr u64 MAP_ADDRESS_MASK = ~0xF;	// Highest 56 bits

			// According to Companion. Purpose is unknown.
			u32* MetaData;

			u8 VirtualChunkCount;
			u8 PhysicalChunkCount;
			u8 MainChunkIndex;

			// Whether this map is allocated dynamically or generated from compiled file.
			// NOTE: This could be wrong name / interpretation because value is never true
			// and most likely used only in 'compiler' builds of rage, just like metadata.
			// Name is only assumed from these facts:
			// - Resource is de-allocated only if value is false;
			// - 'HasMap' function does check for value being false; (can be seen as resource owning the map)
			// - It's placed right above the chunks.
			bool bIsDynamic;

			// In comparison with datResource, contains only destination (allocated) addresses.
			u64 AddressAndShift[128];

			// Whether this map was built from compiled resource.
			bool IsCompiled() const;
			// Generates mini map from original one.
			void GenerateFromMap(const datResourceMap& map);
			// Recreates resource map from this mini map.
			void RegenerateMap(datResourceMap& map) const;
		};

		Map* m_Map;

		// Builds mini map (consumes less memory)
		// We need this map to destruct and defragment (done via re-allocating resource) paged object
		void MakeDefragmentable(const datResourceMap& map) const;

		// Performs de-allocation of all memory chunks that are owned by this resource.
		// NOTE: This includes main chunk or 'this'; Which means: after this function is invoked,
		// accessing 'this' will cause undefined behaviour.
		void FreeMemory(const datResourceMap& map) const;
	public:
		pgBase();
		virtual ~pgBase();

		// Compiler constructor.
		pgBase(const pgBase& other);

		// De-allocates resource memory.
		void Destroy() const;

		// Whether this paged object has internal resource map,
		// which means that this resource is compiled and was built from file.
		bool HasMap() const;

		// Recreates resource map of this resource. If resource has no map, nothing is done.
		void RegenerateMap(datResourceMap& map) const;

		// Unused in final version (overriden only once in grcTexture)
		// They're present only for virtual table completeness.

#if APP_BUILD_2699_16_RELEASE_NO_OPT
		virtual ConstString GetDebugName() { return ""; }
#endif
		virtual u32 GetHandleIndex() const { return 0; }
		virtual void SetHandleIndex(u32 index) { }
	};
}
