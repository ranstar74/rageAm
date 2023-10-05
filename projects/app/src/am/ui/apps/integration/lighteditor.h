//
// File: lighteditor.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "game/drawable.h"

namespace rageam::integration
{
	enum eGimzoMode
	{
		GIZMO_None,
		GIZMO_Translate,
		GIZMO_Rotate,
	};

	class LightEditor
	{
		rage::atArray<rage::Mat44V> m_LightWorldMatrices;
		int							m_SelectedLight = -1;
		int							m_GizmoMode = GIZMO_None;

	public:
		void Render(gtaDrawable* drawable, const rage::Mat44V& entityMtx);
	};
}
