//
// File: assetstores.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "assetstore.h"
#include "streamengine.h"
#include "game/drawable.h"

namespace rage
{
	// TODO: In native code each store have it's own type

	// TODO:
	// Def' structs are generally the same but there can be difference -
	// for example Unknown10 field in fwDrawableDef.
	// We need a better way to handle it.

	typedef struct fwAssetDef
	{
		pgBase*			Object;
		s32				RefCount;
		u32				NameHash;
		strLocalIndex	DependencyTXD; // Archetype sets it to specified TXD slot in ITYP 
		u32				Flags;

		void DestroyObject()
		{
			delete Object;
			Object = nullptr;
		}
	} fwTxdDef, fwDwdDef, fwFragmentDef;

	struct fwDrawableDef
	{
		pgBase*			Object;
		u64				Unknown10;
		s32				RefCount;
		u32				NameHash;
		strLocalIndex	DependencyTXD;
		u32				Flags;

		void DestroyObject()
		{
			delete Object;
			Object = nullptr;
		}
	};

	inline auto GetTxdStore()
	{
		return (fwAssetStore<pgDictionary<grcTextureDX11>, fwTxdDef>*)strStreamingModuleMgr::GetModule("ytd");
	}
	inline auto GetDrawableStore()
	{
		return (fwAssetStore<gtaDrawable, fwDrawableDef>*)strStreamingModuleMgr::GetModule("ydr");
	}
	inline auto GetDrawableDictionaryStore()
	{
		return (fwAssetStore<pgDictionary<gtaDrawable>, fwDwdDef>*)strStreamingModuleMgr::GetModule("ydd");
	}
	inline auto GetFragmentStore()
	{
		// TODO: Type is placeholder
		return (fwAssetStore<char, fwFragmentDef>*)strStreamingModuleMgr::GetModule("yft");
	}
}
