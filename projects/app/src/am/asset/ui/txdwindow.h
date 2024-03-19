//
// File: txdwindow.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/asset/types/texpresets.h"
#include "am/asset/types/txd.h"
#include "am/asset/ui/assetwindow.h"
#include "am/graphics/image/image.h"
#include "am/system/worker.h"
#include "am/ui/slwidgets.h"

namespace rageam::ui
{
	class TxdWindow;

	// Moved out because shared between txd and preset editors
	bool RenderCompressOptionsControls(graphics::ImageCompressorOptions& options, bool compressorAvailable, float itemWidth);

	class TextureVM
	{
		struct AsyncImage
		{
			amComPtr<ID3D11ShaderResourceView>	View;
			// We can't set view from parallel thread, it's still might be referenced in the draw list
			amComPtr<ID3D11ShaderResourceView>	ViewPending;
			graphics::ImagePtr					Image;
			BackgroundTaskPtr					LoadTask;
			graphics::ImageCompressorToken		LoadToken;
			graphics::CompressedImageInfo		CompressedInfo = {};	// Only if loaded compressed
			u64									LastLoadTime = 0;
			ImVec2								UV2 = { 1, 1 };			// Only for RAW view, since we allow weird-sized RGBA
			bool								IsLoading;
			std::recursive_mutex				Mutex;					// Must be locked before accessing anything

			~AsyncImage();

			void LoadAsyncCompressed(const file::WPath& path, const graphics::ImageCompressorOptions& options);
			void LoadAsync(const file::WPath& path);
			void CancelAsyncLoading();

			ImTextureID GetTextureID();
		};

		struct SamplerCallbackState
		{
			int LOD;
			int IsLinear;
		};
		static_assert(sizeof SamplerCallbackState <= 8);

		struct SharedState
		{
			// Samplers are PER LOD because in each sampler we define mip index as min/max
			amComPtr<ID3D11SamplerState>		LodPointSamplers[graphics::IMAGE_MAX_MIP_MAPS];
			amComPtr<ID3D11SamplerState>		LodLinearSamplers[graphics::IMAGE_MAX_MIP_MAPS];
			amComPtr<ID3D11SamplerState>		CheckerSampler; // Point (nearest)
			amComPtr<ID3D11SamplerState>		DefaultSampler; // Linear
			// Alpha gray-white checker, for viewport background
			amComPtr<ID3D11ShaderResourceView>	Checker;

			SharedState();

			// We use lazy loading for samplers, creating them at the same time is wasting resources
			// and time-consuming, so we create one by one when they're needed
			ID3D11SamplerState* GetSamplerStateFor(int mipLevel, bool linear);
		};
		static inline amUPtr<SharedState>	sm_SharedState;

		TxdWindow*							m_Parent;
		asset::TextureTune*					m_Tune;
		file::Path							m_TextureName;
		// We store pending options to properly handle sliders
		graphics::ImageCompressorOptions	m_CompressOptionsPending;
		graphics::ImageCompressorOptions	m_CompressOptions;
		bool								m_CompressOptionsMatchPreset = false;
		asset::TexturePresetPtr				m_MatchedTexturePreset;
		int									m_MipLevel = 0;
		bool								m_DisplayCompressed = true;
		// Unlocks extra options in UI, such as mip map level and sampler
		bool								m_AdvancedView = false;
		// Only used when m_AdvancedView is true
		bool								m_PointSampler = true;
		ImVec2								m_HoveredCoors = {};
		amUPtr<AsyncImage>					m_Raw;
		amUPtr<AsyncImage>					m_Compressed;
		// 'Projection' - shift and depth
		Vec3S								m_ImageView = { 0, 0, 1 };
		Vec3S								m_ImageViewOld = m_ImageView;
		ImVec2								m_HalfViewportSize = {};
		// Image was renamed / deleted
		bool								m_IsMissing = false;
		// For settings copying, flag if selected in list
		bool								m_MarkedForCopying = false;

		ImVec2 WorldToScreen(ImVec2 pos) const;
		ImVec2 ScreenToWorld(ImVec2 pos) const;

		// Two ImGui callbacks to set samplers for viewport textures
		static void BackupSamplerCallback(const ImDrawList*, const ImDrawCmd*);
		static void RestoreSamplerCallback(const ImDrawList*, const ImDrawCmd*);
		static void SetSamplerCallback(const ImDrawList*, const ImDrawCmd* cmd);
		static void SetCheckerSamplerCallback(const ImDrawList*, const ImDrawCmd*);

		void SetCompressOptionsWithUndo(const graphics::ImageCompressorOptions& newCompressOptions);

	public:
		TextureVM(TxdWindow* parent, asset::TextureTune* tune);

		const graphics::ImageCompressorOptions& GetCompressorOptions() const { return m_CompressOptions; }

		bool RenderListEntry(bool selected) const;
		void RenderMiscProperties();
		void RenderImageProperties();
		void RenderHoveredPixel(const graphics::ImagePtr& image) const;
		void RenderImageInfo(const graphics::ImagePtr& image) const;
		void RenderLoadingProgress(const amUPtr<AsyncImage>& im) const;
		void RenderImageViewport();
		void ResetViewport();
		void GetIcon(ImTextureID& id, int& width, int& height) const;
		void UpdateTune() const;

		void ReloadImage(bool onlyCompressed = false) const;

		asset::TextureTune* GetTune() const { return m_Tune; }
		ConstString GetTextureName() const { return m_TextureName; }
		// In case if file path was changed in tune
		void UpdateTextureName();

		bool IsMissing() const { return m_IsMissing; }
		void SetIsMissing(bool missing);

		void ReloadPreset();
		void SetPreset(const asset::TexturePresetPtr& preset);

		static void ShutdownClass();
	};

	class TexturePresetWizard
	{
		struct PresetEntry
		{
			static constexpr int NAME_MAX = 32;
			static constexpr int ARG_MAX = 32;

			asset::TexturePreset	Preset;
			bool					IsDeleted;		// Preset marked for deletion
			char					Name[NAME_MAX];
			char					Argument[ARG_MAX];
		};

		int								m_SelectedIndex = 0;
		int								m_AliveEntriesCount = 0; // Entries that are not marked for deletion
		UndoStack						m_Undo;
		ConstWString					m_WorkspacePath;
		List<PresetEntry>				m_Entries;
		asset::TexturePresetCollection* m_PresetCollection = nullptr;
		List<string>					m_ValidationErrors;

		// For safety and synchronization reasons we edit preset copies,
		// then if user decides to save data - we write them back
		void SnapshotPresets();

		void RenderPresetList();
		void RenderPresetUI();

		void ResetSelectionToFirstAlive();

		void DoSave() const;
		bool ValidateEntries();

	public:
		TexturePresetWizard(ConstWString workspacePath);

		// selectedOptions are for copying from selected texture
		// Return true if preset changes were saved
		bool Render(const graphics::ImageCompressorOptions* selectedOptions);
	};

	/**
	 * \brief UI Editor for Texture Dictionary.
	 */
	class TxdWindow : public AssetWindow
	{
		friend class TextureVM;

		List<TextureVM>				m_TextureVMs;
		int							m_SelectedIndex;
		amUPtr<TexturePresetWizard>	m_TexturePresetWizard;

		void RenderTextureList();
		void LoadTexturesFromAsset();

		void OnFileChanged(const file::DirectoryChange& change) override;
		void OnRender() override;
		void OnAssetMenuRender() override;

		// Synchronizes changes with hot drawable (or well, loaded game scene)
		// so user can instantly preview changes
		void NotifyHotDrawableThatConfigWasChanged();

		TextureVM& GetVMFromTune(const asset::TextureTune& tune);
		TextureVM& GetVMFromHashKey(u32 hashKey);

	public:
		TxdWindow(const asset::AssetPtr& asset);
		~TxdWindow() override;

		void SaveChanges() override;
		void Reset() override;
		void OnRefresh() override;

		asset::TxdAssetPtr GetAsset();

		ImVec2 GetDefaultSize() override { return { 740, 350 }; }
	};
}
