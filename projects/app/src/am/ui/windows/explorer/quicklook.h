//
// File: quicklook.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "entry.h"
#include "am/ui/image.h"

namespace rageam::ui
{
	class QuickLookType
	{
	protected:
		ExplorerEntryPtr m_Entry;
	public:
		QuickLookType(const ExplorerEntryPtr& entry) : m_Entry(entry) {}
		virtual ~QuickLookType() = default;

		virtual void Render() = 0;
		virtual bool IsLoaded() const = 0;
	};

	class QuickLookImage : public QuickLookType
	{
		Image image;
	public:
		QuickLookImage(const ExplorerEntryPtr& entry);

		void Render() override;
		bool IsLoaded() const override { return image.IsLoaded(); }
	};

	/**
	 * \brief OSX-Like tool to preview files in explorer.
	 */
	class QuickLook
	{
		static constexpr ConstString POPUP_NAME = "QUICK_VIEW_POPUP";
		static constexpr float OPEN_ANIM_SPEED = 20.0f;

		// Opening animation: Window slowly resizes from void to it's actual size
		// This might be junky algorithm but there's no way to set transform in ImGui
		//
		// Step 1: When popup is opened, we run window twice - first time all components
		//  are placed and emitting cursor is set to final position, using that information
		//  on second time ImGui fill's window size. During that 'layout' pass we have
		//  window fully transparent, non visible to user.
		// Step 2: Now final window size is measured and we can start interpolating it until
		//  it reaches actual window size.

		ImVec2 m_WindowSize;
		ImVec2 m_AnimatedSize;
		bool m_Measured = false;
		bool m_Opening = false;

		amUniquePtr<QuickLookType> m_LookType;
	public:
		void Open(const ExplorerEntryPtr& entry);
		void Close();
		bool IsOpened() const;

		void Render();
	};
}
