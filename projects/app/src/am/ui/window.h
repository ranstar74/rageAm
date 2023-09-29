//
// File: window.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "am/task/undo.h"
#include "imgui.h"

namespace rageam::ui
{
	/**
	 * \brief UI Window with unique title.
	 */
	class Window
	{
	public:
		virtual ~Window() = default;

		virtual bool HasMenu() const { return false; }
		virtual void OnMenuRender() {}
		virtual void OnRender() = 0;
		virtual bool IsDisabled() const { return false; }
		virtual bool ShowUnsavedChangesInTitle() const { return false; }
		virtual ConstString GetTitle() const = 0;
		virtual ImVec2 GetDefaultSize() { return { 0, 0 }; }

		UndoStack Undo;
	};
}
