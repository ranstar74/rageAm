#include "thread.h"

bool rageam::render::Thread::UpdateAndCallRenderFn()
{
	if (m_StopRequested)
		return false;
	
	if (!m_RenderFn())
	{
		m_StopRequested = true;
		return false;
	}

	return true;
}

rageam::render::Thread::Thread(const TRenderFn& renderFn)
{
	m_RenderFn = renderFn;
}
