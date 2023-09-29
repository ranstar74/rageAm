#pragma once

#include "common/logger.h"
#include "common/types.h"

// Enables checking of allocator state
#ifdef _DEBUG
//#define ENABLE_ALLOCATOR_SANITY_CHECK
#endif

// Enables ::BeginMemoryLog which prints memory operations, stacktrace and such
#ifdef _DEBUG
//#define ENABLE_ALLOCATOR_OPERATION_LOG
#endif

// Enables detailed allocator log
// #define ENABLE_ALLOCATOR_LOG

// Enables printing state of the allocator after operations.
#ifdef ENABLE_ALLOCATOR_LOG
#define ALLOC_PRINT_STATE
#endif

// Log macro definition
#ifdef ENABLE_ALLOCATOR_LOG
#include "common/logger.h"
#define ALLOC_LOG(fmt, ...) AM_TRACEF(fmt, __VA_ARGS__)
#else
inline void AllocLogPlaceHolder(ConstString, ...) {}
#define ALLOC_LOG(fmt, ...) AllocLogPlaceHolder(fmt, __VA_ARGS__)
#endif

namespace rage
{
	class sysMemAllocator;

	// About 1 GB. Derived from smallocator.
	static constexpr u64 SYS_GENERAL_MAX_HEAP_SIZE = 0x4000'0000;

	static constexpr u8 SYS_MEM_MAX_MEMORY_BUCKETS = 16;
	static constexpr u8 SYS_MEM_INVALID_BUCKET = -1;

	inline rageam::Logger* GetMemoryLogger()
	{
		static rageam::Logger logger("memory");
		return &logger;
	}

	/**
	 * \brief Used to divide allocated memory blocks on 16 distinct buckets.
	 * \n NOTE: Value has to be in range from 0 to 15 inclusive.
	 */
	inline u8& GetCurrentMemoryBucket()
	{
		thread_local u8 sysMemCurrentMemoryBucket = 0;
		return sysMemCurrentMemoryBucket;
	}

	// For more info, see sysMemMultiAllocator::m_Allocators
	enum eAllocatorType
	{
		ALLOC_TYPE_GENERAL = 0,
		ALLOC_TYPE_VIRTUAL = 1,
		ALLOC_TYPE_PHYSICAL = 2, // Additionally mapped to 3 but for enum continuous range 2 is used
	};

	class sysMemAllocator
	{
		static inline sysMemAllocator* sm_Current = nullptr;
	public:
		sysMemAllocator() = default;
		virtual ~sysMemAllocator() = default;

		static sysMemAllocator* GetCurrent() { return sm_Current; }
		static void SetCurrent(sysMemAllocator* that) { sm_Current = that; }

		/**
		 * \brief Sets whether allocator should throw on errors or ignore them.
		 * \return Previous toggle state.
		 */
		virtual bool SetQuitOnFail(bool toggle) { return false; }

		virtual pVoid Allocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) = 0;
		virtual pVoid TryAllocate(u64 size, u64 align = 16, u32 type = ALLOC_TYPE_GENERAL) = 0;

		virtual void Free(pVoid block) = 0;

	private:
		// This function is not overloaded in any allocator and just calls free.
		// Purpose is unknown so I leave it private.
		virtual void TryFree(pVoid block) { Free(block); }
	public:

		/**
		 * \brief Only shrinks allocated block, allowing allocator to reuse unused memory.
		 * \n NOTE: New size has to be less or equal to current block size!
		 */
		virtual void Resize(pVoid block, u64 newSize) {}

		/**
		 * \brief For multi allocator, gets specific allocator by type index.
		 * \n Known ones are specified in rage::eAllocatorType enumeration.
		 */
		virtual const sysMemAllocator* GetAllocator(u32 type) const { return this; }

		/**
		 * \brief For multi allocator, gets specific allocator by type index.
		 * \n Known ones are specified in rage::eAllocatorType enumeration.
		 */
		virtual sysMemAllocator* GetAllocator(u32 type) { return this; }

		/**
		 * \brief Gets allocator instance that owns the pointer, added for multi-allocator.
		 */
		virtual sysMemAllocator* GetPointerOwner(pVoid block) { return IsValidPointer(block) ? this : nullptr; }

		/**
		 * \brief Gets size of allocated block from pointer.
		 */
		virtual u64 GetSize(pVoid block) { return 0; }

		/**
		 * \brief Gets memory used by particular bucket (see ::GetCurrentMemoryBucket),
		 * or if memoryBucket set to -1, by whole allocator.
		 * \n In addition with ::GetMemorySnapshot used to analyze memory allocations
		 * of algorithm or some subsystem.
		 */
		virtual u64 GetMemoryUsed(u8 memoryBucket = SYS_MEM_INVALID_BUCKET) = 0;

		virtual u64 GetMemoryAvailable() = 0;

		/**
		 * \brief For multi allocator, allocator type is always ALLOC_TYPE_GENERAL.
		 */
		virtual u64 GetLargestAvailableBlock() { return 0; }

		/**
		 * \brief Gets lowest available memory since beginning of measurement.
		 * \param updateMeasure Whether to reset measure or not.
		 */
		virtual u64 GetLowWaterMark(bool updateMeasure = false) { return 0; }

		/**
		 * \brief Gets highest available memory since beginning of measurement
		 * \param updateMeasure Whether to reset measure or not.
		 */
		virtual u64 GetHighWaterMark(bool updateMeasure) { return 0; }

		/**
		 * \brief Backups currently allocated memory stats (see ::GetCurrentMemoryBucket).
		 */
		virtual void UpdateMemorySnapshots() {}

		/**
		 * \brief Gets memory used from bucket snapshot.
		 * \n This used to get memory footprint of particular function or class.
		 * (see ::GetCurrentMemoryBucket for more details)
		 * \param memoryBucket Value in range from 0 to 15 inclusive.
		 */
		virtual u64 GetMemorySnapshot(u8 memoryBucket) { return 0; }

		// TODO: Both simple / buddy allocator are 'tailed'. What is this for? Definitely not about block headers.
		virtual bool IsTailed() { return true; }

		/**
		 * \brief Begins new memory layer, which is used to track down memory leaks.
		 * \n For example, layers could be: 'Global', 'Compiler', 'Collision Test'.
		 * \n After layer is created, every new allocation is belongs to him and to layers created later.
		 * \return AllocID counter at the moment of creation of the layer.
		 */
		virtual u64 BeginLayer() { return 0; }

		/**
		 * \brief Ends up layer created earlier and prints out memory leaks.
		 * \n NOTE: Layers must be closed in stack order!
		 * \param layerName Name of the layer in log, for example 'Global'.
		 * \param logName Path to file where leaked AllocID's will be printed to.
		 */
		virtual void EndLayer(const char* layerName, const char* logName) {}

		/**
		 * \brief Begins printing operation details in file.
		 * \param logName Name of the file where log will be written to.
		 * \param traceStack Whether to print stack trace or not, might be computationally expensive.
		 */
		virtual void BeginMemoryLog(const char* logName, bool traceStack = true) {}
		virtual void EndMemoryLog() {}

		virtual bool IsBuildingResource() { return false; }
		virtual bool HasMemoryBuckets() { return false; }

		/**
		 * \brief Ensured that the state of the allocator is valid, debug only.
		 * \n Automatically performed on alloc, free, resize operations.
		 */
		virtual void SanityCheck() {}

		/**
		 * \brief Gets whether this block is owned by this allocator and in allocated state.
		 */
		virtual bool IsValidPointer(pVoid block) { return true; }

	private:
		// Not implemented / Debug only

		virtual bool SupportsAllocateMap() { return false; }
		virtual bool AllocateMap() { return false; }
		virtual void FreeMap() { }
	public:
		/**
		 * \brief Gets size of block including header size,
		 * or simply how much memory it consumes in total.
		 */
		virtual u64 GetSizeWithOverhead(pVoid block)
		{
			return GetSize(block); // No overhead
		}

		virtual u64 GetHeapSize() { return 0; }
		virtual pVoid GetHeapBase() { return nullptr; }
		virtual void SetHeapBase(pVoid newBase) { }

	private:
		// Not implemented / Debug only

		virtual bool IsRootResourceAllocation() { return false; }
	public:

		// TODO:
		// GetCanonicalBlockPtr
		// TryLockBlock
		// UnblockBlock
		// GetUserData
		// SetUserData
		// GetMemoryDistribution
		// Defragment
		// GetFragmentation
	};

	class sysScopedLayer
	{
		sysMemAllocator* m_Allocator;
		ConstString	m_Name;
		ConstString	m_LogPath;
	public:
		sysScopedLayer(sysMemAllocator* allocator, ConstString name, ConstString logPath)
		{
			m_Allocator = allocator;
			m_Name = name;
			m_LogPath = logPath;

			m_Allocator->BeginLayer();
		}

		~sysScopedLayer()
		{
			m_Allocator->EndLayer(m_Name, m_LogPath);
		}
	};
}
