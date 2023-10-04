#include "viewport.h"

#include "am/desktop/window.h"
#include "am/integration/memory/address.h"
#include "rage/grcore/device.h"
#include "rage/math/math.h"

u64 GetGameViewport()
{
	static gmAddress viewports_Addr = gmAddress::Scan("48 8B 0D ?? ?? ?? ?? 33 DB 48 85 C9 74 2B").GetRef(3);
	u64 viewportGame_Addr = *viewports_Addr.To<u64*>();
	return viewportGame_Addr + 0x10; // CViewportGame::m_Viewport -> rage::grcViewport
}

void CViewport::GetCamera(rage::Vec3V& front, rage::Vec3V& right, rage::Vec3V& up)
{
	rage::Mat44V viewInverse = GetViewMatrix().Inverse();
	front = viewInverse.R[2];
	right = viewInverse.R[0];
	up = viewInverse.R[1];
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

void CViewport::GetWorldSegmentFromScreen(rage::Vec3V& nearCoord, rage::Vec3V& farCoord, float mouseX, float mouseY)
{
	u32 width, height;
	rage::grcDevice::GetScreenSize(width, height);

	float xRelative = mouseX / static_cast<float>(width);
	float yRelative = mouseY / static_cast<float>(height);

	yRelative = 1.0f - yRelative; // Flip Y axis

	// Convert to projected screen coordinates (where 0.0, 0.0 is the center)
	float x = rage::Math::Remap(xRelative, 0.0f, 1.0f, -1.0f, 1.0f);
	float y = rage::Math::Remap(yRelative, 0.0f, 1.0f, -1.0f, 1.0f);

	// TODO: Use inverse view proj from viewport
	rage::Mat44V inverseViewProj = GetViewProjectionMatrix().Inverse();

	// Project screen coordinate back in world
	rage::Vec3V screenNear(x, y, 0.0f);
	rage::Vec3V screenFar(x, y, 1.0f);
	nearCoord = screenNear.Transform(inverseViewProj);
	farCoord = screenFar.Transform(inverseViewProj);
}

void CViewport::GetWorldRayFromScreen(rage::Vec3V& fromCoord, rage::Vec3V& dir, float mouseX, float mouseY)
{
	rage::Vec3V nearCoord, farCoord;
	GetWorldSegmentFromScreen(nearCoord, farCoord, mouseX, mouseY);
	fromCoord = nearCoord;
	dir = (farCoord - nearCoord).Normalized();
}

void CViewport::GetWorldMouseSegment(rage::Vec3V& nearCoord, rage::Vec3V& farCoord)
{
	u32 mouseX, mouseY;

	rageam::Window* window = rageam::WindowFactory::GetWindow();
	window->GetMousePos(mouseX, mouseY);

	GetWorldSegmentFromScreen(nearCoord, farCoord, static_cast<float>(mouseX), static_cast<float>(mouseY));
}

void CViewport::GetWorldMouseRay(rage::Vec3V& fromCoord, rage::Vec3V& dir)
{
	rage::Vec3V nearCoord, farCoord;
	GetWorldMouseSegment(nearCoord, farCoord);
	fromCoord = nearCoord;
	dir = (farCoord - nearCoord).Normalized();
}
