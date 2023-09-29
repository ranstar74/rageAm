#pragma once

#include "streaming.h"
#include "common/types.h"

namespace rage
{
	struct strStreamingInfo
	{
		strIndex	Slot;
		u32			Data;
	};

	// This is really on early stages, don't use!

	class strStreamingModuleMgr
	{
	public:

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
