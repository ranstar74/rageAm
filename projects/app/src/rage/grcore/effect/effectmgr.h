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
		static u32 GetEffectCount()
		{
			static gmAddress findEffectByHashKey_Addr = gmAddress::Scan("48 89 5C 24 08 4C 63 1D ?? ?? ?? ?? 48");
			return *findEffectByHashKey_Addr.GetRef(5 + 3).To<u32*>();
		}

		static grcEffect* GetEffect(u32 index)
		{
			static gmAddress findEffectByHashKey_Addr = gmAddress::Scan("48 89 5C 24 08 4C 63 1D ?? ?? ?? ?? 48");
			grcEffect** effects = findEffectByHashKey_Addr.GetRef(0xC + 3).To<grcEffect**>();
			return effects[index];
		}

		static grcEffect* FindEffectByHashKey(u32 hashKey)
		{
			static gmAddress findEffectByHashKey_Addr = gmAddress::Scan("48 89 5C 24 08 4C 63 1D ?? ?? ?? ?? 48");
			static grcEffect* (*fn)(u32) = findEffectByHashKey_Addr.To<decltype(fn)>();
			return fn(hashKey);
		}

		static grcEffect* FindEffect(ConstString name) { return FindEffectByHashKey(joaat(name)); }

		static bool LoadFromFile(grcEffect* effect, ConstString filePath)
		{
			// TODO: Calls effect destructor?

			fiStreamPtr stream = fiStream::Open(filePath);
			// TODO: Preload

			effect->m_FileTime = fiGetFileTime(filePath);

			u32 magic = stream->ReadU32();
			constexpr u32 m = FOURCC('r', 'g', 'x', 'e');
			if (magic != m)
			{
				AM_ERRF("grcEffectMgr::LoadFromFile() -> Invalid effect file, magic didn't match: %u", magic);
				return false;
			}

			effect->FromStream(stream, "normal_spec");
			return true;
		}
	};
}
