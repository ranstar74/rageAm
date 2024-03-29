#include "viewport.h"

#include "am/integration/memory/address.h"
#include "am/graphics/window.h"
#include "rage/grcore/device.h"
#include "rage/math/math.h"

u64 GetGameViewport()
{
#if APP_BUILD_2699_16_RELEASE_NO_OPT
	static gmAddress viewports_Addr = gmAddress::Scan("48 8D 0D ?? ?? ?? ?? 48 03 C8 48 8B C1 48 89 44 24 38 0F 57 C0").GetRef(3).GetAt(0xB00);
#else
	static gmAddress viewports_Addr = gmAddress::Scan("48 8B 0D ?? ?? ?? ?? 33 DB 48 85 C9 74 2B").GetRef(3);
#endif
	u64 viewportGame_Addr = *viewports_Addr.To<u64*>();
	return viewportGame_Addr + 0x10; // CViewportGame::m_Viewport -> rage::grcViewport
}

void CViewport::GetCamera(rage::Vec3V* front, rage::Vec3V* right, rage::Vec3V* up, rage::Vec3V* pos)
{
	rage::Mat44V viewInverse = GetViewMatrix().Inverse();
	// Remember that view matrix reorders components in order to output projected screen coordinate
	// Camera front is Z (depth), right is X and up is Y
	if (front) *front = -viewInverse.R[2];
	if (right) *right = viewInverse.R[0];
	if (up) *up = viewInverse.R[1];
	if (pos) *pos = viewInverse.R[3];
}

rage::Vec3V CViewport::GetCameraPos()
{
	rage::Vec3V pos;
	GetCamera(nullptr, nullptr, nullptr, &pos);
	return pos;
}

rage::Mat44V CViewport::GetCameraMatrix()
{
	rage::Mat34V camera;
	GetCamera(&camera.Front, &camera.Right, &camera.Up, &camera.Pos);
	return camera.To44();
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
	float x = rage::Remap(xRelative, 0.0f, 1.0f, -1.0f, 1.0f);
	float y = rage::Remap(yRelative, 0.0f, 1.0f, -1.0f, 1.0f);

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
	int mouseX, mouseY;

	rageam::graphics::Window* window = rageam::graphics::Window::GetInstance();
	window->GetMousePosition(mouseX, mouseY);

	GetWorldSegmentFromScreen(nearCoord, farCoord, static_cast<float>(mouseX), static_cast<float>(mouseY));
}

void CViewport::GetWorldMouseRay(rage::Vec3V& fromCoord, rage::Vec3V& dir)
{
	rage::Vec3V nearCoord, farCoord;
	GetWorldMouseSegment(nearCoord, farCoord);
	fromCoord = nearCoord;
	dir = (farCoord - nearCoord).Normalized();
}
