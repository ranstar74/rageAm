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

#endif
