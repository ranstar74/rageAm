//
// File: singleton.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "asserts.h"

namespace rageam
{
	/**
	 * \brief Class with single static instance.
	 */
	template<typename T>
	class Singleton
	{
		static inline T* sm_Instance = nullptr;

	public:
		Singleton()
		{
			AM_ASSERT(sm_Instance == nullptr, "Singleton() -> Another instance already exists!");
			sm_Instance = (T*)this;
		}

		virtual ~Singleton()
		{
			sm_Instance = nullptr;
		}

		static T* GetInstance() { return sm_Instance; }
	};
}
