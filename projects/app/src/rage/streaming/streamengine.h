//
// File: streamengine.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "streaming.h"
#include "common/types.h"

namespace rage
{
	class strStreamingModule;

	struct strStreamingInfo
	{
		strIndex	Slot;
		u32			Data;
	};

	// This is really on early stages, don't use!

	class strStreamingModuleMgr
	{
	public:
		// Extension must be in platform-specific format, for example: 'ydr', 'ytd'
		static strStreamingModule* GetModule(ConstString extension);
	};

	class strStreamingInfoManager
	{
		// strStreamingInfo* m_Infos;
	public:
		strStreamingInfo* GetInfo(strIndex slot) const { return nullptr; }
		strStreamingInfoManager* GetModuleMgr() const { return nullptr; }
	};

	class strStreamingEngine
	{
	public:

	};
}
