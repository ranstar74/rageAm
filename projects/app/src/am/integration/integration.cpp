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
	rageam::integration::GameIntegration* s_Instance = nullptr;
}

static bool(*CApp_GameUpdate_gImpl)();
bool CApp_GameUpdate_aImpl()
{
	// Nothing to update
	if (!s_Instance || !s_Instance->ComponentMgr->HasAnyComponent())
		return  CApp_GameUpdate_gImpl();

	s_Instance->ComponentMgr->EarlyUpdateAll();
	bool result = CApp_GameUpdate_gImpl();
	s_Instance->ComponentMgr->LateUpdateAll();
	return result;
}

void rageam::integration::GameIntegration::InitComponentManager()
{
	ComponentMgr = std::make_unique<ComponentManager>();
	ComponentManager::SetCurrent(ComponentMgr.get());

	// CApp::GameUpdate, main game thread
	s_GameUpdateAddr =
		gmAddress::Scan("48 83 EC 28 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? B9");
	Hook::Create(s_GameUpdateAddr, CApp_GameUpdate_aImpl, &CApp_GameUpdate_gImpl, true);
}

void rageam::integration::GameIntegration::ShutdownComponentManager() const
{
	// Component manager will pause current thread and continue
	// updating until every component is fully aborted
	ComponentMgr->BeginAbortAll();

	Hook::Remove(s_GameUpdateAddr);
	ComponentManager::SetCurrent(nullptr);
}

void rageam::integration::GameIntegration::RegisterApps() const
{
	Gui->Apps.AddApp(new StarBar());
	Gui->Apps.AddApp(new ModelScene());
}

rageam::integration::GameIntegration::GameIntegration()
{
	InitComponentManager();
	RegisterApps();
}

rageam::integration::GameIntegration::~GameIntegration()
{
	ShutdownComponentManager();
}

rageam::integration::GameIntegration* rageam::integration::GameIntegration::GetInstance()
{
	return s_Instance;
}

void rageam::integration::GameIntegration::SetInstance(GameIntegration* instance)
{
	s_Instance = instance;
}

#endif
