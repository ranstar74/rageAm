#include "GameInput.h"

#include "am/integration/memory/address.h"

void (*gImpl_DisableAllControlActions)(s32 padIndex);

void hooks::GameInput::Init()
{
	gImpl_DisableAllControlActions = gmAddress::Scan("40 53 48 83 EC 20 33 DB 85 C9 75 09").To<decltype(gImpl_DisableAllControlActions)>();
}

void hooks::GameInput::DisableAllControlsThisFrame()
{
	gImpl_DisableAllControlActions(0);
}
