//
// File: viewport.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/math/mtxv.h"

class CViewport
{
public:
	static void GetCamera(rage::Vec3V& front, rage::Vec3V& right, rage::Vec3V& up);
	static const rage::Mat44V& GetViewMatrix();
	static const rage::Mat44V& GetViewProjectionMatrix();
	static const rage::Mat44V& GetProjectionMatrix();
	static void GetWorldSegmentFromScreen(rage::Vec3V& nearCoord, rage::Vec3V& farCoord, float mouseX, float mouseY);
	static void GetWorldRayFromScreen(rage::Vec3V& fromCoord, rage::Vec3V& dir, float mouseX, float mouseY);
	// Gets ray in world coordinates from mouse cursor
	static void GetWorldMouseSegment(rage::Vec3V& nearCoord, rage::Vec3V& farCoord);
	static void GetWorldMouseRay(rage::Vec3V& fromCoord, rage::Vec3V& dir);
};
