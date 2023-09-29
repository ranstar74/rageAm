#pragma once

#include "archetype.h"
#include "am/integration/memory/address.h"

class CModelInfo
{
public:
	static void RegisterModelInfo(CBaseModelInfo* modelInfo)
	{
		static gmAddress addr = gmAddress::Scan("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 48 83 3D ?? ?? ?? ?? ?? 8B FA");
		static void(*fn)(CBaseModelInfo*, int) = addr.To<decltype(fn)>();
		fn(modelInfo, 0);
	}
};
