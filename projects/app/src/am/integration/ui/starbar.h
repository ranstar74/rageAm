//
// File: starbar.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/types.h"
#include "am/system/nullable.h"
#include "am/ui/app.h"

namespace rageam::integration
{
	class ModelScene;

	class StarBar : public ui::App
	{
		Nullable<Mat44V>	m_PreviousCameraMatrix;
		bool				m_CameraEnabled = false;
		bool				m_UseOrbitCamera = true;
		ModelScene*			m_ModelScene = nullptr;

		// Recreates camera component based on set options
		void UpdateCamera() const;
		void OnStart() override;
		void OnRender() override;
	};
}

#endif
