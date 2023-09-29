//
// File: renderer.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include <d3d11.h>
#include "am/desktop/window.h"

namespace rageam::ui
{
	/**
	 * \brief Handles draw/backend related things for ImGui.
	 */
	class Renderer : EventAwareBase
	{
		ID3D11RenderTargetView* m_RenderTarget = nullptr;

		void CreateRT();
		void DestroyRT();
	public:
		Renderer();
		
		void DestroyContext() const;

		void InitImGuiBackEnds() const;

		void BeginFrame() const;
		void EndFrame() const;
	};
}
