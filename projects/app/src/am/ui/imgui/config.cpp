#include "config.h"

#include "common/logger.h"
#include "am/system/debugger.h"
#include "am/system/errordisplay.h"
#include "am/ui/context.h"

#undef RenderArrow

AM_NOINLINE void rageam::ImAssertHandler(bool expression, ConstString assert)
{
	if (expression)
		return;

	if (Gui) // Null if error happened during initialization
	{
		ui::App* lastUpdatedApp = Gui->Apps.GetLastUpdated();
		if (lastUpdatedApp)
			AM_ERRF("ImGui Assert failed while updating %s", lastUpdatedApp->GetDebugName());
	}

	ErrorDisplay::ImAssert(assert, 1 /* This */);
	Debugger::Break();
}
