//
// File: system.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/asset/types/texpresets.h"
#include "am/graphics/image/imagecache.h"
#include "am/graphics/render.h"
#include "am/graphics/window.h"
#include "am/ui/imglue.h"
#include "threadinfo.h"

#ifdef AM_INTEGRATED
#include "am/integration/integration.h"
#include "am/integration/memory/address.h"
#endif

namespace rageam
{
	namespace asset
	{
		class TexturePresetStore;
	}

	struct SystemData
	{
		struct
		{
			int X, Y, Width, Height;
			bool Maximized;
		} Window;

		struct
		{
			int FontSize;
			struct
			{
				List<ConstWString> QuickAccessDirs;
			} Explorer;
		} UI;
	};

	/**
	 * \brief Cares of core system components, such as - memory allocator, exception handler, rendering.
	 */
	class System : public Singleton<System>
	{
		static constexpr ConstWString DATA_FILE_NAME = L"Data.xml";

#ifdef AM_INTEGRATED
		amUPtr<integration::GameIntegration> m_Integration;
		amUPtr<gmAddressCache>				 m_AddressCache;
#endif

		ThreadInfo                        m_ThreadInfo;
		amUPtr<BackgroundWorker>          m_MainWorker;
		amUPtr<asset::TexturePresetStore> m_TexturePresetManager;
		amUPtr<graphics::Window>          m_PlatformWindow;
		amUPtr<graphics::Render>          m_Render;
		amUPtr<graphics::ImageCache>      m_ImageCache;
		amUPtr<ui::ImGlue>                m_ImGlue;
		bool                              m_UseWindowRender = false;
		bool                              m_Initialized = false;

		void LoadDataFromXML();
		void SaveDataToXML() const;

	public:
		System() = default;
		~System() override { Destroy(); }

		void Destroy();
		// Non ui option available if we want command line application
		void Init(bool withUI);

		void Update() const;

		SystemData	Data = {};
		bool		HasData = false; // Was data loaded from XML file this session?
	};
}
