//
// File: collisionflags.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"
#include "helpers/flagset.h"

namespace rage
{
	// Real names can be found here, but not all hashes are solved:
	// https://alexguirre.github.io/rage-parser-dumps/dump.html?game=rdr3&build=1491&search=collisionf#eCollisionFlags

	enum eCollisionFlags : u32
	{
		CF_NONE = 0u,
		CF_UNKNOWN = 1u,
		CF_MAP_WEAPON = 1u << 1,
		CF_MAP_DYNAMIC = 1u << 2,
		CF_MAP_ANIMAL = 1u << 3,
		CF_MAP_COVER = 1u << 4,
		CF_MAP_VEHICLE = 1u << 5,
		CF_VEHICLE_NOT_BVH = 1u << 6,
		CF_VEHICLE_BVH = 1u << 7,
		CF_VEHICLE_BOX = 1u << 8,
		CF_PED = 1u << 9,
		CF_RAGDOLL = 1u << 10,
		CF_ANIMAL = 1u << 11,
		CF_ANIMAL_RAGDOLL = 1u << 12,
		CF_OBJECT = 1u << 13,
		CF_OBJECT_ENV_CLOTH = 1u << 14,
		CF_PLANT = 1u << 15,
		CF_PROJECTILE = 1u << 16,
		CF_EXPLOSION = 1u << 17,
		CF_PICKUP = 1u << 18,
		CF_FOLIAGE = 1u << 19,
		CF_FORKLIFT_FORKS = 1u << 20,
		CF_TEST_WEAPON = 1u << 21,
		CF_TEST_CAMERA = 1u << 22,
		CF_TEST_AI = 1u << 23,
		CF_TEST_SCRIPT = 1u << 24,
		CF_TEST_VEHICLE_WHEEL = 1u << 25,
		CF_GLASS = 1u << 26,
		CF_MAP_RIVER = 1u << 27,
		CF_SMOKE = 1u << 28,
		CF_UNSMASHED = 1u << 29,
		CF_MAP_STAIRS = 1u << 30,
		CF_MAP_DEEP_SURFACE = 1u << 31,
	};

	using CollisionFlags = FlagSet<eCollisionFlags>;
}
