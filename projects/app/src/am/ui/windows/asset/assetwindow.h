#pragma once

#include "am/asset/gameasset.h"
#include "am/ui/window.h"
#include "assethelper.h"

namespace rageam::ui::assetview
{
	class AssetWindow : public Window
	{
		static constexpr ConstString SAVE_POPUP_NAME = "Exporting...##ASSET_SAVE_MODAL_DIALOG";

		using Messages = rage::atArray<rage::atString>;

		std::atomic_bool	m_IsCompiling;
		std::atomic_bool	m_IsSaving;
		asset::AssetPtr		m_Asset;
		char				m_Title[MAX_ASSET_WINDOW_NAME];

		std::mutex			m_ProgressMutex;
		double				m_Progress = 0;
		Messages			m_ProgressMessages;

		void SaveAsync();
		void CompileAsync();
		void OnMenuRender() override;

	public:
		AssetWindow(const asset::AssetPtr& asset)
		{
			m_Asset = asset;
			MakeAssetWindowName(m_Title, asset);
		}

		virtual void OnAssetMenuRender() { }
		virtual void SaveChanges() { }

		asset::AssetPtr& GetAsset() { return m_Asset; }
		bool HasMenu() const override { return true; }
		bool ShowUnsavedChangesInTitle() const override { return true; }
		ConstString GetTitle() const override { return m_Title; }
	};
}
