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
	static const Vec3V SCENE_ISOLATED_POS = { -1700, -6000, 310 };
	static const Vec3V SCENE_DEFAULT_POS = { 0, 0, 400 };

	class StarBar : public ui::App
	{
		Nullable<Mat44V> m_PreviousCameraMatrix;
		CameraComponent  m_Camera;
		bool             m_CameraEnabled = false;
		bool             m_UseOrbitCamera = true;
		bool             m_UseIsolatedScene = false;
		float			 m_SceneRotation = 0.0f;

		void UpdateCamera();
		void OnRender() override;
		void OnStart() override;

	public:
		void SetCameraEnabled(bool b)
		{
			m_CameraEnabled = b;
			UpdateCamera();
		}

		// == Toggleable options ==

		// Focuses camera component on loaded 3D scene, and
		// creates a new camera if scene is not loaded
		static inline bool FocusCameraOnScene = true;
	};
}

#endif
