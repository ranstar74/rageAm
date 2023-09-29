//
// File: updatecomponents.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once
#ifdef AM_INTEGRATED

#include "rage/atl/fixedarray.h"

namespace rageam::integration
{
	static constexpr int MAX_UPDATE_COMPONENTS = 64;

	/**
	 * \brief Provides interface for two update functions that will be called before (early) and after (late)
	 * updating game. Called by main thread and it's the only thread where you can safely invoke natives.
	 * \remarks Component must destruct all script resources manually using scrInvoke function in destructor.
	 */
	class IUpdateComponent
	{
		static rage::atFixedArray<IUpdateComponent*, MAX_UPDATE_COMPONENTS> sm_UpdateComponents;

		bool m_Initialized = false;
	public:
		IUpdateComponent();
		virtual ~IUpdateComponent();

		virtual void OnStart() {}
		virtual void OnEarlyUpdate() {}
		virtual void OnLateUpdate() {}

		// Those two are reserved, don't call
		static void EarlyUpdateAll();
		static void LateUpdateAll();
	};
}
#endif
