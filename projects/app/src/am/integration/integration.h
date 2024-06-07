//
// File: integration.h
//
// Copyright (C) 2023-2024 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/system/singleton.h"
#include "am/system/ptr.h"
#include "am/graphics/outlinerender.h"
#include "am/gizmo/gizmobase.h"

namespace rageam::integration
{
	class GameDrawLists;
	class ComponentManager;

	//                                Render ends (GPU present)
	//                                │   GPU is waiting for update 'Safe Area' 
	//                                │   │   Render begins
	//                                │   │   │
	//                                ▼   ▼   ▼
	// Rendering         ##############   #   #################
	// Early Update      #
	// Update                             #
	// Late Update                                            #
	//                   ┌────────────────────────────────────┐
	//                   │            Single Frame            │
	// 
	// Render starts after idle section once draw lists are built,
	// and may going up until next GPU idle section
	// Main game thread performs update during that time in parallel

	class GameIntegration : public Singleton<GameIntegration>
	{
		void HookGameThread() const;
		void HookRenderThread() const;
		void AntiDebugFixes() const;

		amUPtr<ComponentManager>        m_ComponentMgr;
		amUPtr<GameDrawLists>           m_GameDrawLists;
		amUPtr<graphics::OutlineRender> m_OutlineRender;
		amUPtr<gizmo::GizmoManager>     m_GizmoManager;

	public:
		GameIntegration();
		~GameIntegration() override;

		// -- All callbacks in one place, see diagram above --

		void GPUEndFrame();
		void EarlyUpdate() const;
		void Update();
		void LateUpdate() const;
	};
}

#endif
