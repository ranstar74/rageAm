//
// File: explorer.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "folderview.h"
#include "treeview.h"
#include "am/types.h"

namespace rageam::ui
{
	class Explorer : public Window
	{
		TreeView				m_TreeView;
		FolderView				m_FolderView;
		ExplorerEntryUserPtr	m_QuickAccess;
		ExplorerEntryUserPtr	m_ThisPC;

		file::WPath GetExplorerSettingsPath() const;
		void ReadSettings() const;
		void SaveSettings() const;

	public:
		Explorer();
		~Explorer() override;

		void OnStart() override;
		void OnRender() override;
		ConstString GetTitle() const override { return "Explorer"; }
		ImVec2 GetDefaultSize() override { return { 845, 500 }; }
	};
}
