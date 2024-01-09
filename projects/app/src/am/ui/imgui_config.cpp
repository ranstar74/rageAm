#include "imgui_config.h"

#include "imglue.h"
#include "am/system/debugger.h"
#include "am/system/errordisplay.h"

namespace
{
	bool s_AssertThrown = false;
}

AM_NOINLINE void rageam::ImAssertHandler(bool expression, ConstString assert)
{
	if (expression)
		return;

	s_AssertThrown = true;

	ErrorDisplay::ImAssert(assert, 1 /* This */);
	Debugger::BreakIfAttached();
}

bool rageam::ImGetAssertWasThrown()
{
	return s_AssertThrown;
}
