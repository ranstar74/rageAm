#include "shvthread.h"
#ifdef AM_INTEGRATED

#include "integration.h"

#include "am/ui/apps.h"
#include "am/ui/context.h"
#include "am/ui/apps/integration/modelscene.h"
#include "apps/starbar.h"
#include "memory/address.h"
#include "memory/hook.h"

namespace
{
	gmAddress s_GameUpdateAddr;
	gmAddress s_ToRenderFunction;
	rageam::integration::GameIntegration* s_Instance = nullptr;
}

static bool(*CApp_GameUpdate_gImpl)();
bool CApp_GameUpdate_aImpl()
{
	// Nothing to update
	if (!s_Instance || !s_Instance->ComponentMgr->HasAnyComponent())
		return CApp_GameUpdate_gImpl();

	if (s_Instance)
	{
		s_Instance->ComponentMgr->EarlyUpdateAll();
		// s_Instance->FlipDrawListBuffers();
	}
	bool result = CApp_GameUpdate_gImpl();
	if (s_Instance) s_Instance->ComponentMgr->LateUpdateAll();

	return result;
}

static void (*gImpl_DoRenderFunction)(u64 arg);
void aImpl_DoRenderFunction(u64 arg)
{
	if (s_Instance)
	{
		s_Instance->FlipDrawListBuffers();
		if (s_Instance->IsPauseMenuActive())
			s_Instance->ClearDrawLists();
	}
	gImpl_DoRenderFunction(arg);
}

void rageam::integration::GameIntegration::InitComponentManager()
{
	ComponentMgr = std::make_unique<ComponentManager>();
	ComponentManager::SetCurrent(ComponentMgr.get());

	// CApp::GameUpdate, main game thread
	s_GameUpdateAddr =
		gmAddress::Scan("48 83 EC 28 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? B9");
	s_ToRenderFunction =
		gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 48 8D 99 B0 00 00 00 48 8B F9 48 83");
	Hook::Create(s_GameUpdateAddr, CApp_GameUpdate_aImpl, &CApp_GameUpdate_gImpl, true);
	Hook::Create(s_ToRenderFunction, aImpl_DoRenderFunction, &gImpl_DoRenderFunction, true);
}

void rageam::integration::GameIntegration::ShutdownComponentManager() const
{
	// Component manager will pause current thread and continue
	// updating until every component is fully aborted
	ComponentMgr->BeginAbortAll();

	Hook::Remove(s_GameUpdateAddr);
	Hook::Remove(s_ToRenderFunction);
	ComponentManager::SetCurrent(nullptr);
}

void rageam::integration::GameIntegration::RegisterApps() const
{
	Gui->Apps.AddApp(new StarBar());
	Gui->Apps.AddApp(new ModelScene());
}

void rageam::integration::GameIntegration::InitializeDrawLists()
{
	DrawListExecutor::SetCurrent(&m_DrawListExecutor);
	m_DrawListEntity.Create();
	m_DrawListEntity->Spawn();
	m_DrawListEntity->AddDrawList(&DrawListGame);
	m_DrawListEntity->AddDrawList(&DrawListGameUnlit);
	m_DrawListEntity->AddDrawList(&DrawListForeground);
	m_DrawListEntity->AddDrawList(&DrawListCollision);
}

rageam::integration::GameIntegration::GameIntegration()
{
	DrawListGameUnlit.Unlit = true;
	DrawListForeground.NoDepth = true;
	DrawListForeground.Unlit = true;

	InitComponentManager();
	RegisterApps();
	InitializeDrawLists();
}

rageam::integration::GameIntegration::~GameIntegration()
{
	ShutdownComponentManager();
	DrawListExecutor::SetCurrent(nullptr);
}

rageam::integration::GameIntegration* rageam::integration::GameIntegration::GetInstance()
{
	return s_Instance;
}

void rageam::integration::GameIntegration::SetInstance(GameIntegration* instance)
{
	s_Instance = instance;
}

bool rageam::integration::GameIntegration::IsPauseMenuActive() const
{
	// Use by is pause menu active native
	static auto fn = gmAddress::Scan("48 89 5C 24 08 57 48 83 EC 20 80 3D ?? ?? ?? ?? ?? 8B F9 75")
		.ToFunc<bool(int)>();
	return fn(1);
}

#endif
