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
			if (!g_StreamingModuleMgr)
			{
#if APP_BUILD_2699_16_RELEASE_NO_OPT
				g_StreamingModuleMgr = gmAddress::Scan("48 8D 05 ?? ?? ?? ?? 48 05 20 02 00 00 48 8B 94 24 80 00 00 00")
					.GetRef(3).GetAt(0x220);
#else
				g_StreamingModuleMgr = gmAddress::Scan("48 8D 0D ?? ?? ?? ?? 44 0F 44 C0 45 88 43 18")
					.GetRef(3);
#endif
			}
		}

		static rage::strStreamingModule* GetModule(ConstString extension)
		{
			Init();

			static gmAddress addr = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
				"48 89 54 24 10 48 89 4C 24 08 48 83 EC 68 33 D2 48 8B 4C 24 78 E8 ?? ?? ?? ?? 89 44 24 28 48 8D",
#elif APP_BUILD_2699_16_RELEASE
				"40 53 48 83 EC 20 48 8B C2 48 8B D9 33 D2 48 8B C8 E8 ?? ?? ?? ?? 8B 0D",
#else
				"40 53 48 83 EC 20 48 8B C2 48 8B D9 33 D2 48 8B C8 E8 ?? ?? ?? ?? 33",
#endif
				"rage::strStreamingModuleMgr::GetModuleFromFileExtension");
			static auto fn = addr.ToFunc<rage::strStreamingModule * (u64, ConstString)>();
			return fn(g_StreamingModuleMgr, extension);
		}

		static rage::strStreamingInfo* GetInfo(rage::strIndex index)
		{
			Init();
			static rage::strStreamingInfo* infos =
				*gmAddress::Scan("48 8B 0D ?? ?? ?? ?? 41 03 C1")
				.GetRef(3)
				.To<decltype(infos)*>();

			if (index == rage::INVALID_STR_INDEX)
				return nullptr;

			return infos + index.Get();
		}
	};
}
