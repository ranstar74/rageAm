#ifndef AM_STANDALONE

#include "gamehooks.h"

#include "memory/hook.h"

#include "hooks/errordialog.h"
#include "hooks/antidebug.h"
#include "hooks/gameviewport.h"
#include "hooks/GameInput.h"
#include "hooks/streaming.h"

void GameHooks::InitFromGtaThread()
{
	//rageam::integration::ScriptHook::Start();
}

void GameHooks::Init()
{
	Hook::Init();

	//hooks::ErrorDialog::Init();
	//hooks::AntiDebug::Init();
	//hooks::GameViewport::Init();
	hooks::GameInput::Init();
	hooks::Streaming::Init();

	//rageam::integration::ScriptHook::Init();
}

void GameHooks::Shutdown()
{
	// TODO: We can't call it from here
	// hooks::GameViewport::Shutdown();

	//rageam::integration::ScriptHook::Shutdown();

	Hook::Shutdown();
}

void GameHooks::EnableAll()
{
	Hook::Seal();
}

#endif
