#pragma once

enum BPCondition
{
    BP_Write = 1,
    BP_ReadWrite = 3
};

namespace HWBreakpoint
{

	bool Set(void* address, BPCondition when, int length);
	void Clear(void* address);
	void ClearAll();
};
