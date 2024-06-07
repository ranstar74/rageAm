//
// File: types.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "rage/math/vec.h"

enum scrControlType
{
	scrControlType_Player,
	scrControlType_Camera,
	scrControlType_Frontend,
};

enum scrRenderingOptionFlags
{
	RO_NO_OPTIONS = 0,
	RO_STOP_RENDERING_OPTION_WHEN_PLAYER_EXITS_INTO_COVER = 1
};

// Defines the rotation order to be used for Euler composition and decomposition
enum scrEulerRotOrder
{
	EULER_XYZ,
	EULER_XZY,
	EULER_YXZ,
	EULER_YZX,
	EULER_ZXY,
	EULER_ZYX,
	EULER_MAX
};

enum scrShapetestStatus
{
	SHAPETEST_STATUS_NONEXISTENT = 0,	// Shapetest requests are discarded if they are ignored for a frame or as soon as the results are returned
	SHAPETEST_STATUS_RESULTS_NOTREADY,	// Not ready yet; try again next frame
	SHAPETEST_STATUS_RESULTS_READY		// The result is ready and the results have been returned to you. The shapetest request has also just been destroyed
};

#define SCRIPT_INCLUDE_MOVER		1
#define	SCRIPT_INCLUDE_VEHICLE		2
#define	SCRIPT_INCLUDE_PED			4
#define SCRIPT_INCLUDE_RAGDOLL		8
#define	SCRIPT_INCLUDE_OBJECT		16
#define SCRIPT_INCLUDE_PICKUP		32
#define SCRIPT_INCLUDE_GLASS		64
#define SCRIPT_INCLUDE_RIVER		128
#define SCRIPT_INCLUDE_FOLIAGE		256
#define SCRIPT_INCLUDE_ALL			511

#define SCRIPT_SHAPETEST_OPTION_IGNORE_GLASS		1
#define SCRIPT_SHAPETEST_OPTION_IGNORE_SEE_THROUGH	2
#define SCRIPT_SHAPETEST_OPTION_IGNORE_NO_COLLISION	4
#define SCRIPT_SHAPETEST_OPTION_DEFAULT	SCRIPT_SHAPETEST_OPTION_IGNORE_GLASS | SCRIPT_SHAPETEST_OPTION_IGNORE_SEE_THROUGH | SCRIPT_SHAPETEST_OPTION_IGNORE_NO_COLLISION

// Prevents accidental implicit cast
template<typename T = int, T Default = T()>
class scrHandle
{
	T m_Value = Default;
public:
	scrHandle() = default;
	scrHandle(const scrHandle& other) = default;
	explicit scrHandle(T v) { m_Value = v; }

	bool IsValid() const { return m_Value != Default; }
	void Reset() { m_Value = Default; }
	T    Get() const { return m_Value; }
	void Set(T v) { m_Value = v; }

	scrHandle& operator=(const scrHandle&) = default;
	operator bool() const { return m_Value; }
	operator T() const { return m_Value; }
	bool operator !() const { return !m_Value; }
};

using scrVector = rage::Vector3;

struct scrThreadId			: scrHandle<> { using scrHandle::scrHandle; };
struct scrCameraIndex		: scrHandle<int, -1> { using scrHandle::scrHandle; };
struct scrObjectIndex		: scrHandle<> { using scrHandle::scrHandle; };
struct scrShapetestIndex	: scrHandle<> { using scrHandle::scrHandle; };
struct scrBlipIndex			: scrObjectIndex { using scrObjectIndex::scrObjectIndex; };
struct scrPedIndex			: scrObjectIndex { using scrObjectIndex::scrObjectIndex; };
struct scrEntityIndex		: scrObjectIndex { using scrObjectIndex::scrObjectIndex; };
