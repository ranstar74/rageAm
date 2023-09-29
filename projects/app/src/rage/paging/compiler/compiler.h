//
// File: compiler.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "packer.h"
#include "writer.h"
#include "snapshotallocator.h"

#include "common/logger.h"
#include "am/system/timer.h"

namespace rage
{
	using ResourceCompileCallback = void(ConstWString message, double percent);

	class pgRscCompiler
	{
		// Keep synced with system/systemheap.h limits

		static constexpr u32 VIRTUAL_ALLOCATOR_SIZE = 30ull * 1024ull * 1024ull;	// 30MB
		static constexpr u32 PHYSICAL_ALLOCATOR_SIZE = 160ull * 1024ull * 1024ull;	// 160MB

		static inline thread_local pgRscCompiler* tl_Compiler = nullptr;

		pgSnapshotAllocator* m_VirtualAllocator = nullptr;
		pgSnapshotAllocator* m_PhysicalAllocator = nullptr;

		pgBase* m_RootResourceAllocation = nullptr;

		// Replaces pointers on file offsets
		void FixupReferences(const datPackedChunks& pack, const pgSnapshotAllocator& allocator) const;

		template<typename TPaged>
		TPaged* DoSnapshot(const TPaged* pPaged)
		{
			// 'Compiling' resource is done through literally copying it

			pVoid block = m_VirtualAllocator->Allocate(sizeof(TPaged));
			m_RootResourceAllocation = (pgBase*)block;
			return new (block) TPaged(*pPaged);
		}

		template<typename TPaged>
		bool DoCompile(const TPaged* pPaged, u32 version, const wchar_t* path)
		{
			bool success = true;

			pgSnapshotAllocator virtualAllocator(VIRTUAL_ALLOCATOR_SIZE, true);
			pgSnapshotAllocator physicalAllocator(PHYSICAL_ALLOCATOR_SIZE, false);
			m_VirtualAllocator = &virtualAllocator;
			m_PhysicalAllocator = &physicalAllocator;

			// Step 1: Make snapshot of all structures to virtual / physical allocators.
			ReportProgress(L"Performing memory snapshot", 0.0);
			DoSnapshot(pPaged);

			// Step 2: Pack all allocations (allocator chunks) to pages.
			ReportProgress(L"Packing chunks", 0.3);
			datCompileData data{};
			data.Version = version;

			// Possible fail reasons:
			//  - Block size is larger than 100MB (pgStreamer limit)
			pgRscPacker virtualPacker(virtualAllocator, 0);
			if (success && !virtualPacker.Pack(data.VirtualChunks)) success = false;

			pgRscPacker physicalPacker(physicalAllocator, data.VirtualChunks.ChunkCount);
			if (success && !physicalPacker.Pack(data.PhysicalChunks)) success = false;

			if (success)
			{
				// Step 3: Now order of memory blocks finalized and will remain unchanged,
				// we have to replace pointer addresses on file offsets
				ReportProgress(L"Fixing up pointers", 0.7);
				FixupReferences(data.VirtualChunks, virtualAllocator);
				FixupReferences(data.PhysicalChunks, physicalAllocator);

				// Step 4: Compress and write data to file.
				ReportProgress(L"Writing to file", 0.9);
				pgRscWriter writer;

				// Possible fail reasons:
				//  - Unable to open file for writing
				if (!writer.Write(path, data)) success = false;
			}

			m_VirtualAllocator = nullptr;
			m_PhysicalAllocator = nullptr;

			return success;
		}
	public:
		// Invoked during various stages of resource compiling
		// This function is thread-safe (invoked only from caller thread)
		std::function<ResourceCompileCallback> CompileCallback;

		void ReportProgress(ConstWString message, double progress) const
		{
			if (!CompileCallback)
				return;
			CompileCallback(message, progress);
		}

		/**
		 * \brief Performs resource compilation (copying) to file.
		 * \tparam TPaged	Type of root resource structure.
		 * \param pPaged	Pointer to root resource structure.
		 * \param version	Version of resource from paging/builder/pgBuilds.h;
		 * \param path		Path to output file.
		 */
		template<typename TPaged>
		bool Compile(const TPaged* pPaged, u32 version, const wchar_t* path)
		{
			tl_Compiler = this;

			static rageam::Logger logger("resource_compiler");
			rageam::LoggerScoped scopedLogger(logger);

			sysScopedLayer layer(GetMultiAllocator(), "Resource Compiler", "resource_compiler_leaks");

			AM_TRACEF(L"pgRscCompiler::Compile() -> Compiling resource %ls", path);

			rageam::Timer timer = rageam::Timer::StartNew();
			bool result = DoCompile(pPaged, version, path);
			timer.Stop();

			if (result)
				AM_TRACEF("pgRscCompiler::Compile() -> Finished job in %llums", timer.GetElapsedMilliseconds());
			else
				AM_ERRF("pgRscCompiler::Compile() -> Job finished with errors");

			tl_Compiler = nullptr;
			return result;
		}

		// Gets whether given pgBase instance is root of the resource
		// We need this in cases when resource is composed of multiple structs that inherit pgBase
		// such as rmcDrawable -> crSkeletonData, but we have to generate map only for rmcDrawable pgBase
		static bool IsRootResourceAllocation(const pgBase* base)
		{
			pgRscCompiler* compiler = GetCurrent();
			if (!compiler)
				return false;

			return base == compiler->m_RootResourceAllocation;
		}

		static pgSnapshotAllocator* GetVirtualAllocator() { return tl_Compiler ? tl_Compiler->m_VirtualAllocator : nullptr; }
		static pgSnapshotAllocator* GetPhysicalAllocator() { return tl_Compiler ? tl_Compiler->m_PhysicalAllocator : nullptr; }

		static pgRscCompiler* GetCurrent() { return tl_Compiler; }
	};
}
