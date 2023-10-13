//
// File: context.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "am/desktop/input.h"
#include "am/ui/apps.h"
#include "am/ui/icons.h"
#include "am/task/undo.h"
#include "apps/statusbar.h"
#include "apps/windowmgr.h"
#include "imgui/renderer.h"

namespace rageam::ui
{
	/**
	 * \brief User interface context. Holds icons, apps and other things.
	 */
	struct UIContext
	{
		Renderer			Renderer;
		Icons				Icons;
		Apps				Apps;
		WindowManager*		Windows;
		StatusBar*			Status;
		Input				Input;

		UIContext();
		~UIContext();

		bool Update();
		void SetupImGui() const;
		void StyleBlack() const;
		void StyleLight() const;
	};
}

// Global user interface context
extern rageam::ui::UIContext* Gui;
std::recursive_mutex& GetGuiMutex();

void CreateUIContext();
void DestroyUIContext();
