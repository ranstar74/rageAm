#include "integration.h"

#include "memory/address.h"
#ifdef AM_INTEGRATED

#include "memory/hook.h"

namespace
{
	gmAddress s_GameUpdateAddr;
	rageam::integration::GameIntegration* s_Instance = nullptr;
}

rageam::integration::GameIntegration* GIntegration = nullptr;

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

rageam::integration::GameIntegration::GameIntegration()
{
	s_Instance = this;

	ComponentMgr = std::make_unique<ComponentManager>();
	ComponentManager::SetCurrent(ComponentMgr.get());

	// CApp::GameUpdate, main game thread
	s_GameUpdateAddr =
		gmAddress::Scan("48 83 EC 28 E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? B9");
	Hook::Create(s_GameUpdateAddr, CApp_GameUpdate_aImpl, &CApp_GameUpdate_gImpl, true);
}

rageam::integration::GameIntegration::~GameIntegration()
{
	// Component manager will pause current thread and continue
	// updating until every component is fully aborted
	ComponentMgr->BeginAbortAll();

	Hook::Remove(s_GameUpdateAddr);
	ComponentManager::SetCurrent(nullptr);
	s_Instance = nullptr;
}

void CreateIntegrationContext()
{
	GIntegration = new rageam::integration::GameIntegration();
}

void DestroyIntegrationContext()
{
	delete GIntegration;
	GIntegration = nullptr;
}
#endif
