#include "streamengine.h"

#include "am/integration/hooks/streaming.h"

rage::strStreamingModule* rage::strStreamingModuleMgr::GetModule(ConstString extension)
{
	return hooks::Streaming::GetModule(extension);
}
