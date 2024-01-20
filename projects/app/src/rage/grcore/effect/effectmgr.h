//
// File: effectmgr.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "effect.h"
#include "common/logger.h"
#include "common/types.h"
#include "rage/file/fiutils.h"
#include "rage/file/stream.h"
#include "helpers/fourcc.h"

namespace rage
{
	class grcEffectMgr
	{
	public:
		static grcEffect* GetEffect(u32 index)
		{
			static grcEffect** sm_Effects = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
				"48 8D 04 C1 48 39 44 24 50")
				.GetAt(0x1A)
#else
				"48 89 5C 24 08 4C 63 1D ?? ?? ?? ?? 48")
				.GetAt(0xC)
#endif
				.GetRef(3).To<grcEffect**>();
			return sm_Effects[index];
		}

		static grcEffect* FindEffectByHashKey(u32 hashKey)
		{
			static auto fn =
#if APP_BUILD_2699_16_RELEASE_NO_OPT
				gmAddress::Scan("48 8D 04 C1 48 39 44 24 50", "rage::grcEffect::LookupEffect+0xB8").GetAt(-0xB8)
#else
				gmAddress::Scan("48 89 5C 24 08 4C 63 1D ?? ?? ?? ?? 48")
#endif
				.ToFunc<grcEffect * (u32)>();
			return fn(hashKey);
		}

		static grcEffect* FindEffect(ConstString name) { return FindEffectByHashKey(atStringHash(name)); }

		static bool LoadFromFile(grcEffect* effect, ConstString filePath)
		{
			//// TODO: Calls effect destructor?

			//fiStreamPtr stream = fiStream::Open(filePath);
			//// TODO: Preload

			//effect->m_FileTime = fiGetFileTime(filePath);

			//u32 magic = stream->ReadU32();
			//constexpr u32 m = FOURCC('r', 'g', 'x', 'e');
			//if (magic != m)
			//{
			//	AM_ERRF("grcEffectMgr::LoadFromFile() -> Invalid effect file, magic didn't match: %u", magic);
			//	return false;
			//}

			//effect->FromStream(stream, "normal_spec");
			return true;
		}
	};
}
