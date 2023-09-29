#pragma once

#include "am/integration/memory/address.h"
#include "rage/streaming/streamingmodule.h"
#include "common/types.h"
#include "rage/streaming/streamengine.h"

namespace hooks
{
	class Streaming
	{
		static inline gmAddress g_StreamingModuleMgr;
	public:
		static void Init()
		{
			g_StreamingModuleMgr = gmAddress::Scan("48 8D 0D ?? ?? ?? ?? 44 0F 44 C0 45 88 43 18")
				.GetRef(3);
		}

		static rage::strStreamingModule* GetModule(ConstString extension)
		{
			static rage::strStreamingModule* (*getModuleByExtension)(u64, ConstString) = gmAddress::Scan(
				"40 53 48 83 EC 20 48 8B C2 48 8B D9 33 D2 48 8B C8 E8 ?? ?? ?? ?? 33",
				"rage::strStreamingModuleMgr::GetModuleByExtension")
				.To<decltype(getModuleByExtension)>();

			return getModuleByExtension(g_StreamingModuleMgr, extension);
		}

		static rage::strStreamingInfo* GetInfo(rage::strIndex index)
		{
			static rage::strStreamingInfo* infos =
				*gmAddress::Scan("48 8B 0D ?? ?? ?? ?? 41 03 C1")
				.GetRef(3)
				.To<decltype(infos)*>();

			if (index == rage::INVALID_STR_INDEX)
				return nullptr;

			return infos + index;
		}
	};
}
