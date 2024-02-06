//
// File: extensions.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#ifdef AM_INTEGRATED

#include "core.h"

inline scrVector scrGetGroundCoors(const scrVector& coors)
{
	scrVector groundCoors = coors;
	scrGetGroundZFor3DCoord(groundCoors, groundCoors.Z, true);
	return groundCoors;
}

inline void scrWarpPlayer(const scrVector& coors)
{
	scrSetPedCoordsKeepVehicle(scrPlayerGetID(), scrGetGroundCoors(coors));
}

inline float scrGetTimeFloat()
{
	int h = scrGetClockHours();
	int m = scrGetClockMinutes();
	int s = scrGetClockSeconds();

	float t = 0.0f;
	t += static_cast<float>(h) / 24.0f;
	t += static_cast<float>(m) / 60.0f / 24.0f;
	t += static_cast<float>(s) / 60.0f / 60.0f / 24.0f;

	return t;
}

inline void scrSetTimeFloat(float t)
{
	int h = static_cast<int>(floorf(t * 24.0f));
	int m = static_cast<int>(floorf(fmodf(t, 1.0f / 24.0f) * 60.0f * 24.0f));
	int s = static_cast<int>(floorf(fmodf(t, 1.0f / 24.0f / 60.0f) * 60.0f * 60.0f * 24.0f));

	scrSetClockTime(h, m, s);
}

#endif
