//
// File: imglue.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/types.h"
#include "am/system/singleton.h"
#include "image.h"
#include "app.h"
#include "window.h"

enum ImFonts
{
	ImFont_Regular,
	ImFont_Small,
	ImFont_Medium,
	ImFont_BANK,
};

enum ImStyles
{
	ImStyle_VS,
	ImStyle_Dark,
	ImStyle_Light,
	ImStyle_Classic,
};

enum ImExtraFontRanges_
{
	ImExtraFontRanges_None = 0,
	ImExtraFontRanges_Cyrillic = 1 << 1,
	ImExtraFontRanges_Chinese = 1 << 2,
	ImExtraFontRanges_Japanese = 1 << 3,
	ImExtraFontRanges_Greek = 1 << 4,
	ImExtraFontRanges_Thai = 1 << 5,
	ImExtraFontRanges_Korean = 1 << 6,
	ImExtraFontRanges_Vietnamese = 1 << 7,
};
typedef int ImExtraFontRanges;

namespace ImGui
{
	void PushFont(ImFonts font);
}

namespace rageam::ui
{
	static constexpr int UI_MAX_ICON_RESLUTION = 256;

	class ImGlue : public Singleton<ImGlue>
	{
		// To detect if font needs to be rebuilt
		int					m_OldFontSize = FontSize;
		int					m_OldExtraFontRanges = ExtraFontRanges;
		u32					m_OldAccentColor = 0;
		ImStyles			m_OldStyle = Style;

		HashSet<ImImage>	m_Icons;
		List<amUPtr<App>>	m_Apps;
		App*				m_LastUpdatedApp = nullptr;
		ImImage				m_NotFoundImage;
		bool				m_Enabled = false;
		std::atomic_bool	m_IsFirstFrame = true;
		mutable std::mutex	m_Mutex;

		void CreateContext() const;
		void DestroyContext() const;

		// We use accent color for font icon luminosity transformation
		// (in order to adjust dynamically for a theme)
		u32 GetThemeAccentColor() const;

		// Adds icons from 'data/font_icons' into given font
		void EmbedFontIcons(ImFonts font) const;
		// Extra languages that we want to load from font file
		void BuildFontRanges(List<ImWchar>& fontRanges) const;
		void CreateDefaultFonts(const ImWchar* fontRanges) const;
		// Creates famous font used in rage BANK widgets
		// Font has fixed size 8x8 pixels and can't be resized
		void CreateBankFont() const;
		void CreateFonts() const;

		void RegisterSystemApps();

		// Loads .ICO and .PNG icons from 'data/icons'
		void LoadIcons();

		void StyleVS() const;

	public:
		ImGlue();
		~ImGlue() override;

		// Must be called from update thread
		bool BuildDrawList();
		// Renders draw list that was built in BuildDrawList, must be called from render thread
		void Render() const;
		// Updates ImGui platform windows, must be called from window thread
		void PlatformUpdate() const;

		// UI is disabled by default, game update is initialized before rendering,
		// and we have to make sure that we start pumping UI only after both render and game update are init
		void SetEnabled(bool enabled) { m_Enabled = enabled; }

		// App must be allocated via operator new
		void AddApp(App* app);
		// Only valid during frame execution
		App* GetLastUpdatedApp() const { return m_LastUpdatedApp; }
		template<typename T>
		T* FindAppByType()
		{
			for (amUPtr<App>& app : m_Apps)
			{
				T* obj = dynamic_cast<T*>(app.get());
				if (obj) return obj;
			}
			return nullptr;
		}
		
		ImImage* GetIcon(ConstString name) { return GetIconByHash(Hash(name)); }
		ImImage* GetIconByHash(HashValue hash);

		int					FontSize = 16;
		bool				IsDisabled = false; // UI will appear and function as disabled
		bool				IsVisible = true;	// UI rendering will be skipped, only OnUpdate function will be called for apps
		ImExtraFontRanges	ExtraFontRanges = 0;
		ImStyles			Style = ImStyle_VS;
		WindowManager*		Windows;
		Timer				LastUpdateTimer;	// Draw list building time (update function)
	};

	inline ImGlue* GetUI() { return ImGlue::GetInstance(); }
}
