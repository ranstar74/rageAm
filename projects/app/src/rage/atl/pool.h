//
// File: pool.h
//
// Copyright (C) 2023 ranstar74. All rights violated.
//
// Part of "Rage Am" Research Project.
//
#pragma once

#include "common/types.h"

namespace rage
{
	class atPoolBase
	{
		struct FreeNode
		{
			FreeNode* Next;
		};

		char* m_Storage;
		u64 m_TotalCount;
		u64 m_FreeCount;
		u64 m_ElementSize;
		FreeNode* m_FreeHead;
		bool m_OwnMemory;
		u32* m_Infos;
		u16 m_InfoSize;
		u16 m_Size;
	};
}
