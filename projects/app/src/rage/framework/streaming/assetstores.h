//
// File: assetstores.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "assetstore.h"
#include "game/drawable.h"
#include "rage/atl/rangearray.h"
#include "rage/streaming/streamengine.h"

namespace rage
{
	class gtaFragType;
	class fwMapTypesContents;

	// In game code every store have derived class, but we don't really need to reimplement them

	typedef pgDictionary<grcTexture>  fwTxd;
	typedef pgDictionary<gtaDrawable> fwDwd;
	typedef gtaDrawable               fwDrawable;
	typedef gtaFragType               fwFragment;

	struct fwTxdDef : fwAssetDef<fwTxd>
	{
		strLocalIndex ParentId;
		u16           IsDummy          : 1;
		u16           IsBeingDefragged : 1;
		u16           HasHD            : 1;
		u16           IsBoundHD        : 1;
		u16           Padding          : 12;
	};
	typedef fwAssetStore<fwTxd, fwTxdDef> fwTxdStore;

	struct fwDrawableDef : fwAssetDef<fwDrawable>
	{
		strLocalIndex TxdIndex;
		u16           MatchError       : 1;
		u16           IsBeingDefragged : 1;
		u16           HasHD            : 1;
		u16           IsBoundHD        : 1;
		u16           Padding          : 12;
	};
	typedef fwAssetStore<fwDrawable, fwDrawableDef> fwDrawableStore;

	struct fwDwdDef : fwAssetDef<fwDwd>
	{
		strLocalIndex TxdIndex;
		u16           NoAssignedTxd    : 1;
		u16           MatchError       : 1;
		u16           IsBeingDefragged : 1;
		u16           HasHD            : 1;
		u16           IsBoundHD        : 1;
		u16           Padding          : 11;
	};
	typedef fwAssetStore<fwDwd, fwDwdDef> fwDwdStore;

	struct fwFragmentDef : fwAssetDef<fwFragment>
	{
		strLocalIndex TxdIndex;
		u16           NoAssignedTxd : 1;
		u16           MatchError : 1;
		u16           IsBeingDefragged : 1;
		u16           HasHD : 1;
		u16           IsBoundHD : 1;
		u16           Padding : 11;
	};
	typedef fwAssetStore<fwFragment, fwFragmentDef> fwFragmentStore;

	struct fwBoundDef
	{
		phBound*     Object;
		atHashString Name;
		u16          RefCount;
		u16          Padding        : 13;
		u16          IsDummy        : 1;
		u16          IsDependency   : 1;
		u16          HasRiverBounds : 1;
	};
	typedef fwAssetStore<phBound, fwBoundDef> fwStaticBoundsStore;

	struct fwMapTypesDef : fwAssetDef<fwMapTypesContents>
	{
		static constexpr int MAX_TYPE_DEPS = 8;

		struct
		{
			u16 Initialised     : 1;
			u16 IsNativeFile    : 1;
			u16 IsPermanent     : 1; // marked as permanent through the .meta file
			u16 IsDependency    : 1; // marked as an .imap dependency through ._manifest file in .rpf
			u16 IsPermanentDLC  : 1; // is part of a DLC package and is permanent while the pack is active
			u16 IsPso           : 1;
			u16 IsFastPso       : 1;
			u16 IsInPlace       : 1;
			u16 DelayLoading    : 1; // debug : for .ityp files which can reference other .ityp files (e.g. interiors)
			u16 ContainsMLOtype : 1;
			u16 Unused          : 6;
		} Flags;
		u16 TypeDataIndicesCount;
		union
		{
			s32                               Index;
			atRangeArray<s32, MAX_TYPE_DEPS>* Indices; // type data dependency for these instances
		} TypeDataIndex;
	};
	typedef fwAssetStore<fwMapTypesContents, fwMapTypesDef> fwMapTypesStore;

	inline auto GetStore(ConstString fileExtension) { return strStreamingModuleMgr::GetModule(fileExtension); }
	inline auto GetBoundStore()    { return static_cast<fwStaticBoundsStore*>(GetStore("ybn")); }
	inline auto GetTxdStore()      { return static_cast<fwTxdStore*>(GetStore("ytd")); }
	inline auto GetDrawableStore() { return static_cast<fwDrawableStore*>(GetStore("ydr")); }
	inline auto GetDwdStore()      { return static_cast<fwDwdStore*>(GetStore("ydd")); }
	inline auto GetFragmentStore() { return static_cast<fwFragmentStore*>(GetStore("yft")); }
	inline auto GetMapTypesStore() { return static_cast<fwMapTypesStore*>(GetStore("ytyp")); }
}
