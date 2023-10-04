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
	class LightEditor
	{
		rage::atArray<rage::Mat44V> m_LightWorldMatrices;
		int							m_SelectedLight = -1;

	public:
		void Render(gtaDrawable* drawable, const rage::Mat44V& entityMtx);
	};
}
