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

namespace rageam::integration
{
	class GameIntegration
	{
		void InitComponentManager();
		void ShutdownComponentManager() const;
		void RegisterApps() const;

	public:
		GameIntegration();
		~GameIntegration();

		static GameIntegration* GetInstance();
		static void SetInstance(GameIntegration* instance);

		amUniquePtr<ComponentManager>		ComponentMgr;
		ComponentOwner<ICameraComponent>	Camera;
	};
}

#endif
