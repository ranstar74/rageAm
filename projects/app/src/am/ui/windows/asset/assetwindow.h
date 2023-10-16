#pragma once

#include "am/asset/gameasset.h"
#include "am/ui/window.h"
#include "assethelper.h"
#include "rage/file/watcher.h"

namespace rageam::ui::assetview
{
	class AssetWindow : public Window
	{
		static constexpr ConstString SAVE_POPUP_NAME = "Exporting...##ASSET_SAVE_MODAL_DIALOG";

		using Messages = rage::atArray<rage::atString>;

		rage::fiDirectoryWatcher	m_Watcher;

		std::atomic_bool			m_IsCompiling;
		std::atomic_bool			m_IsSaving;
		asset::AssetPtr				m_Asset;
		char						m_Title[MAX_ASSET_WINDOW_NAME];

		std::mutex					m_ProgressMutex;
		double						m_Progress = 0;
		Messages					m_ProgressMessages;

		void SaveAsync();
		void CompileAsync();
		void OnMenuRender() override;

	protected:
		void OnRender() override
		{
			if (m_Watcher.GetChangeOccuredAndReset())
			{
				OnFileChanged();
			}
		}

	public:
		AssetWindow(const asset::AssetPtr& asset)
		{
			m_Asset = asset;
			m_Watcher.SetEntry(String::ToUtf8Temp(m_Asset->GetDirectoryPath()));
			MakeAssetWindowName(m_Title, asset);
		}

		virtual void OnFileChanged() { }
		virtual void OnAssetMenuRender() { }
		virtual void SaveChanges() { }

		asset::AssetPtr& GetAsset() { return m_Asset; }
		bool HasMenu() const override { return true; }
		bool ShowUnsavedChangesInTitle() const override { return true; }
		ConstString GetTitle() const override { return m_Title; }
	};
}
