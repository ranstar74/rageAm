#include "viewport.h"
#include "am/integration/memory/address.h"

u64 GetGameViewport()
{
	static gmAddress viewports_Addr = gmAddress::Scan("48 8B 0D ?? ?? ?? ?? 33 DB 48 85 C9 74 2B").GetRef(3);
	u64 viewportGame_Addr = *viewports_Addr.To<u64*>();
	return viewportGame_Addr + 0x10; // CViewportGame::m_Viewport -> rage::grcViewport
}

const rage::Mat44V& CViewport::GetViewMatrix()
{
	// Actually WorldView but World is identity
	return *(rage::Mat44V*)(GetGameViewport() + 0x40);
}

const rage::Mat44V& CViewport::GetViewProjectionMatrix()
{
	// Same as in GetViewMatrix, WorldViewProj
	return *(rage::Mat44V*)(GetGameViewport() + 0x80);
}

const rage::Mat44V& CViewport::GetProjectionMatrix()
{
	return *(rage::Mat44V*)(GetGameViewport() + 0x140);
}
