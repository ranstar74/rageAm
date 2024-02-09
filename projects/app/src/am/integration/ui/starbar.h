//
// File: starbar.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "am/system/nullable.h"
#include "am/ui/app.h"
#include "am/types.h"

#include "am/integration/components/camera.h"

namespace rageam::integration
{
	class ICameraComponent;
	class ModelScene;

	class StarBar : public ui::App
	{
		Nullable<Mat44V> m_PreviousCameraMatrix;
		CameraComponent  m_Camera;
		bool             m_CameraEnabled = false;
		bool             m_UseOrbitCamera = true;
		ModelScene*      m_ModelScene = nullptr;

		// Recreates camera component based on set options
		void UpdateCamera();
		void OnStart() override;
		void OnRender() override;

	public:
		void SetCameraEnabled(bool b)
		{
			m_CameraEnabled = b;
			UpdateCamera();
		}
	};
}

#endif
