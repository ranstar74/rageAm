//
// File: integration.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "updatecomponent.h"
#include "components/camera.h"
#include "drawlist.h"
#include "game/modelinfo.h"

namespace rageam::integration
{
	class GameEntity;

	class GameIntegration
	{
		void InitComponentManager();
		void ShutdownComponentManager() const;
		void RegisterApps() const;

		// Fake game entity that we use to render draw list
		ComponentOwner<DrawListDummyEntity>	m_DrawListEntity;
		DrawListExecutor					m_DrawListExecutor;

		void InitializeDrawLists();

	public:
		GameIntegration();
		~GameIntegration();

		static GameIntegration* GetInstance();
		static void SetInstance(GameIntegration* instance);

		void FlipDrawListBuffers()
		{
			DrawListGame.FlipBuffer();
			DrawListGameUnlit.FlipBuffer();
			DrawListForeground.FlipBuffer();
			DrawListCollision.FlipBuffer();
		}

		void ClearDrawLists()
		{
			DrawListGame.Clear();
			DrawListGameUnlit.Clear();
			DrawListForeground.Clear();
			DrawListCollision.Clear();
		}
		
		bool IsPauseMenuActive() const;

		amUniquePtr<ComponentManager>		ComponentMgr;
		ComponentOwner<ICameraComponent>	Camera;
		DrawList							DrawListGame;		// Reflections, no alpha
		DrawList							DrawListGameUnlit;	// No reflections, alpha
		DrawList							DrawListForeground;	// Always on top, no reflections, alpha
		DrawList							DrawListCollision;	// In a different list to let user toggle options
	};
}

#endif
