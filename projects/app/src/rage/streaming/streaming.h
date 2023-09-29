#pragma once

#include "common/types.h"

namespace rage
{
	// NOTE:
	// - strLocalIndex is local index in strStreamingModule (simply index in array/pool)
	// - strIndex is global index in strStreamingEngine::ms_info array
	// Those supposedly had more fields in debug game build, but in release its just single 32 bit index.
	// Another possible reason is that you can't pass strLocalIndex in strIndex (fool protection)

	using strLocalIndex = s32;
	using strIndex = s32;

	static constexpr s32 INVALID_STR_INDEX = -1;
}
