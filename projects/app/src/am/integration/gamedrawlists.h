//
// File: gamedrawlists.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "drawlist.h"
#include "am/system/singleton.h"

namespace rageam::integration
{
	/**
	 * \brief Holds categorized draw lists for different purposes.
	 * \n Drawing is done via draw scene render phase hook after Post FX.
	 */
	class GameDrawLists : public Singleton<GameDrawLists>
	{
		DrawListExecutor m_Executor;

	public:
		GameDrawLists();
		~GameDrawLists() override;

		void EndFrame();

		// Called from the game's draw command
		void ExecuteAll();

		DrawList Game;
		DrawList GameUnlit;
		DrawList Overlay;
		DrawList CollisionMesh;
		DrawList Gizmo;
	};
}
#endif
