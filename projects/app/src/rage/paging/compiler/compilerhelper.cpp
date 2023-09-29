#include "compilerhelper.h"
#include "compiler.h"

bool rage::IsResourceCompiling()
{
	pgSnapshotAllocator* pAllocator = pgRscCompiler::GetVirtualAllocator();
	return pAllocator != nullptr;
}

pVoid rage::pgAlloc(u32 size)
{
	if (IsResourceCompiling())
		return pgRscCompiler::GetVirtualAllocator()->Allocate(size);
	return rage_malloc(size);
}

void rage::pgFree(pVoid block)
{
	if (IsResourceCompiling())
		return;
	rage_free(block);
}
