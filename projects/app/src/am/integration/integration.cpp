#ifdef AM_INTEGRATED

#include "integration.h"

#include "am/ui/imglue.h"
#include "memory/address.h"
#include "memory/hook.h"
#include "script/core.h"
#include "ui/starbar.h"
#include "ui/modelscene.h"

namespace
{
	std::atomic_bool	s_InitializedFromGameThread;
	std::atomic_bool	s_ShuttingDown;
	gmAddress			s_GameUpdateAddr;
	gmAddress			s_DoRenderFunction;
}

bool(*CApp_GameUpdate_gImpl)();
bool CApp_GameUpdate_aImpl()
{
	if (!s_InitializedFromGameThread)
	{
		rageam::integration::scrInit();
		s_InitializedFromGameThread = true;
	}

	if (s_ShuttingDown)
	{
		rageam::integration::scrShutdown();
		bool gameUpdateResult = CApp_GameUpdate_gImpl();
		Hook::Remove(s_GameUpdateAddr);
		s_ShuttingDown = false;
		return gameUpdateResult;
	}

	// Update UI from game thread and build draw lists to render
	auto render = rageam::graphics::Render::GetInstance();
	if (render) render->BuildDrawLists();

	auto instance = rageam::integration::GameIntegration::GetInstance();

	// Nothing to update
	if (!instance || !instance->ComponentMgr->HasAnythingToUpdate())
		return CApp_GameUpdate_gImpl();

	if (instance) instance->ComponentMgr->EarlyUpdateAll();
	bool result = CApp_GameUpdate_gImpl();
	if (instance) instance->ComponentMgr->LateUpdateAll();

	return result;
}

void (*gImpl_DoRenderFunction)(u64 arg);
void aImpl_DoRenderFunction(u64 arg)
{
	auto instance = rageam::integration::GameIntegration::GetInstance();
	if (instance)
	{
		instance->FlipDrawListBuffers();
		if (instance->IsPauseMenuActive())
			instance->ClearDrawLists();
	}
	gImpl_DoRenderFunction(arg);
}

void rageam::integration::GameIntegration::HookGameThreadAndInitComponentManager()
{
	ComponentMgr = std::make_unique<ComponentManager>();

	// CApp::RunGame, main game thread
#if APP_BUILD_2699_16_RELEASE_NO_OPT
	s_GameUpdateAddr =
		gmAddress::Scan("48 89 4C 24 08 48 81 EC 38 02 00 00 48 8D 05 ?? ?? ?? ?? 48");
	s_DoRenderFunction =
		gmAddress::Scan("48 8B 49 08 48 8B 00 8A 54 24 20").GetAt(-0xC3);
#elif APP_BUILD_2699_16_RELEASE
	s_GameUpdateAddr =
		gmAddress::Scan("48 89 5C 24 08 57 48 81 EC 80 01 00 00 33");
	s_DoRenderFunction =
		gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 48 8D 99 B8 00 00 00 48 8B F9 48 83");
#else
	s_GameUpdateAddr =
		gmAddress::Scan("48 83 EC 28 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? B9");
	s_DoRenderFunction =
		gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 48 8D 99 B0 00 00 00 48 8B F9 48 83");
#endif
	Hook::Create(s_GameUpdateAddr, CApp_GameUpdate_aImpl, &CApp_GameUpdate_gImpl);
	Hook::Create(s_DoRenderFunction, aImpl_DoRenderFunction, &gImpl_DoRenderFunction);
}

void rageam::integration::GameIntegration::ShutdownComponentManager()
{
	// Component manager will pause current thread and continue
	// updating until every component is fully aborted
	//ComponentMgr->BeginAbortAll();

	//Hook::Remove(s_GameUpdateAddr);
	Hook::Remove(s_DoRenderFunction);

	ComponentMgr = nullptr;
}

void rageam::integration::GameIntegration::RegisterApps() const
{
	ui::ImGlue* ui = ui::GetUI();
	ui->AddApp(new StarBar());
	ui->AddApp(new ModelScene());
}

void rageam::integration::GameIntegration::InitializeDrawLists()
{
	//m_DrawListEntity.Create();
	//m_DrawListEntity->Spawn();
	//m_DrawListEntity->AddDrawList(&DrawListGame);
	//m_DrawListEntity->AddDrawList(&DrawListGameUnlit);
	//m_DrawListEntity->AddDrawList(&DrawListForeground);
	//m_DrawListEntity->AddDrawList(&DrawListCollision);
}

rageam::integration::GameIntegration::GameIntegration()
{
	HookGameThreadAndInitComponentManager();
	// System is created from DLL injector thread, we must initialize some things
	// from game update thread
	while (!s_InitializedFromGameThread) {}

	DrawListGameUnlit.Unlit = true;
	DrawListForeground.NoDepth = true;
	DrawListForeground.Unlit = true;

	RegisterApps();
	// InitializeDrawLists();
}

rageam::integration::GameIntegration::~GameIntegration()
{
	s_ShuttingDown = true;
	while (s_ShuttingDown) {}

	ShutdownComponentManager();
}

bool rageam::integration::GameIntegration::IsPauseMenuActive() const
{
	// Use by is pause menu active native
	static auto fn = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"89 4C 24 08 48 83 EC 38 0F B6 05 ?? ?? ?? ?? 85 C0 75 15"
#elif APP_BUILD_2699_16_RELEASE
		"48 89 5C 24 08 57 48 83 EC 20 80 3D ?? ?? ?? ?? ?? 8A 05"
#else
		"48 89 5C 24 08 57 48 83 EC 20 80 3D ?? ?? ?? ?? ?? 8B F9 75"
#endif
	).ToFunc<bool(int)>();
	return fn(1);
}

void rageam::integration::GameIntegration::DisableAllControlsThisFrame() const
{
	scrBegin();
	scrDisableAllControlActions(scrControlType_Player);
	scrEnd();
}

#endif
