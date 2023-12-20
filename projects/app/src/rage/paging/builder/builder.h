//
// File: builder.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/string/stringwrapper.h"
#include "rage/paging/resource.h"
#include "rage/paging/resourceinfo.h"
#include "am/system/asserts.h"
#include "am/system/timer.h"
#include "rage/file/device.h"
#include "common/logger.h"

namespace rage
{
	class pgRscBuilder
	{
		static constexpr u32 READ_BUFFER_SIZE = 0x1000000; // 16MB

		// Native implementation uses pgReader which does resource reading in parallel thread,
		// we don't need that yet (because we are not streaming resources in real-time) so we can read resource in caller thread.

		static bool PerformReadInMainThread(ConstString path, u32 version, datResourceMap& map, datResourceInfo& info);
		static void ConstructName(char* buffer, u32 bufferSize, const char* path);

	public:
		static bool ReadAndDecompressChunks(datResourceMap& map, fiDevice* device, ConstString path);

		static pgBase* LoadBuild(ConstString path, u32 version, datResourceMap& map, datResourceInfo& info);
		/**
		 * \brief Frees up physical chunks.
		 */
		static void Cleanup(const datResourceMap& map);

		/**
		 * \brief Loads rage resource file in memory.
		 * \tparam TPaged	Root type of resource, such as pgDictionary<grcTextureDX11>.
		 * \param ppPaged	Pointer to root resource variable. If resource is invalid or loading fails, return value is nullptr.
		 * \param path		Path to resource file, including extension.
		 * \param version	Resource version to resolve.
		 */
		template<typename TPaged>
		static void Load(TPaged** ppPaged, ConstString path, u32 version, bool cleanUpPhysical = true)
		{
			static rageam::Logger logger("resource_builder");
			rageam::Logger::Push(&logger);

			rageam::Timer timer = rageam::Timer::StartNew();

			datResourceMap map;
			datResourceInfo info;
			*ppPaged = (TPaged*)LoadBuild(path, version, map, info);

			if (*ppPaged)
			{
				datResource rsc(map);
				rsc.Place(*ppPaged);
			}
			else
			{
				AM_ERRF("pgRscBuilder::Load(%s, %u) -> Failed to load resource!", path, version);
			}

			if(cleanUpPhysical)
				Cleanup(map);

			timer.Stop();
			AM_TRACEF("pgRscBuilder::Load() -> Job finished in %llums", timer.GetElapsedMilliseconds());

			rageam::Logger::Pop();
		}

		template<typename TPaged>
		static void Load(TPaged** ppPaged, ConstString path, ConstString extension, u32 version)
		{
			// TODO: This is temporal measure, need to research fiAssetManager
			char fullPath[256];
			sprintf_s(fullPath, 256, "%s.%s", path, extension);

			Load(ppPaged, fullPath, version);
		}

		/**
		 * \brief Allocates virtual and physical chunks in heap.
		 * \return Whether it was successful to allocate whole map or not.
		 */
		static bool AllocateMap(datResourceMap& map);
	};
}
