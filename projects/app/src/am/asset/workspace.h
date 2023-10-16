//
// File: workspace.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "gameasset.h"
#include "am/types.h"

namespace rageam::asset
{
	class TxdAsset;
	class DrawableAsset;
	// Workspace were introduced mainly as solution of 'visibility scope' issue,
	// for example what textures do we show to in material editor picker?
	// Logical answer - every texture in project directory, this is what workspace is
	// Eventually workspace will compile into .rpf

	static constexpr ConstWString WORKSPACE_EXT = L".pack";

	// NOTE: We use following abbreviations for asset types:
	// TD - Texture Dictionary
	// DR - Drawable

	// If asset load type is not specified, type will be completely ignored during scanning
	enum eWorkspaceFlags_ : u32
	{
		WF_None = 0,
		WF_LoadTx = 1 << 0,
		WF_LoadDr = 1 << 1,

		WF_LoadAll = WF_LoadTx | WF_LoadDr,
	};
	using eWorkspaceFlags = u32;

	class Workspace
	{
		ConstWString		m_Path;
		SmallList<AssetPtr> m_Assets;
		SmallList<u16>		m_TDs;
		SmallList<u16>		m_DRs;
		u16					m_FailedCount;
		eWorkspaceFlags		m_Flags;

		void ScanRecurse(ConstWString path);

	public:
		Workspace(ConstWString path, eWorkspaceFlags flags = WF_LoadAll);

		void Refresh();

		u16 GetAssetCount() const { return m_Assets.GetSize(); }
		u16 GetTexDictCount() const { return m_TDs.GetSize(); }
		u16 GetDrawableCount() const { return m_DRs.GetSize(); }
		const auto& GetAsset(u16 index) const { return m_Assets[index]; }

		auto GetTexDict(u16 index) const -> const amPtr<TxdAsset>&;
		auto GetDrawable(u16 index) const -> const amPtr<DrawableAsset>&;

		// Tries to get asset's parent workspace, returns NULL if asset is not in workspace
		static amPtr<Workspace> FromAssetPath(ConstWString assetPath, eWorkspaceFlags flags);

		// Gets 'x:/dlc.pack' from 'x:/dlc.pack/props'
		// Returns whether path is within a workspace
		static bool GetParentWorkspacePath(ConstWString assetPath, file::WPath& outPath);

		// A valid workspace is a directory ending with '.pack' extension
		static bool IsWorkspace(ConstWString path);
	};

	using WorkspacePtr = amPtr<Workspace>;
}
