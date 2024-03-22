//
// File: assetwindow.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/file/watcher.h"
#include "am/ui/window.h"
#include "am/asset/gameasset.h"
#include "assetwindowfactory.h"

namespace rageam::ui
{
	class AssetWindow : public Window
	{
		static constexpr ConstString REFRESH_POPUP_NAME = "Confirmation required##ASSET_SAVE_MODAL_DIALOG";
		static constexpr ConstString RESET_POPUP_NAME = "Confirmation required##ASSET_RESET_MODAL_DIALOG";

		file::Watcher		m_Watcher;

		std::atomic_bool	m_IsCompiling;
		asset::AssetPtr		m_Asset;
		char				m_Title[AssetWindowFactory::MAX_ASSET_WINDOW_NAME];

		void DoSave();
		void DoRefresh();
		void CompileAsync();
		void OnMenuRender() override;

	protected:
		void OnRender() override
		{
			file::DirectoryChange change;
			if (m_Watcher.GetNextChange(change))
			{
				OnFileChanged(change);
			}
		}

	public:
		AssetWindow(const asset::AssetPtr& asset) : m_Watcher(asset->GetDirectoryPath(), file::NotifyFlags_All)
		{
			m_Asset = asset;
			AssetWindowFactory::MakeAssetWindowName(m_Title, asset);
		}

		virtual void OnFileChanged(const file::DirectoryChange& change) { }
		virtual void OnAssetMenuRender() { }
		virtual void SaveChanges() { }
		virtual void Reset() { }
		virtual void OnRefresh() { }

		asset::AssetPtr& GetAsset() { return m_Asset; }
		bool HasMenu() const override { return true; }
		bool ShowUnsavedChangesInTitle() const override { return true; }
		ConstString GetTitle() const override { return m_Title; }
	};
}
