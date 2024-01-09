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

// Prevents accidental implicit cast
template<typename T, int GUID>
class scrHandle
{
	T m_Value = {};
public:
	scrHandle() = default;
	scrHandle(const scrHandle& other) = default;
	explicit scrHandle(T v) { m_Value = v; }

	T Get() const { return m_Value; }
	void Set(T v) { m_Value = v; }

	scrHandle& operator=(const scrHandle&) = default;
	operator bool() const { return m_Value; }
	bool operator !() const { return !m_Value; }
};

typedef scrHandle<int, 0>	scrThreadId;
typedef scrHandle<int, 1>	scrObjectIndex;
typedef scrHandle<int, 2>	scrPedIndex;
typedef rage::Vector3		scrVector;
