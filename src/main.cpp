﻿#define BOOST_STACKTRACE_USE_NOOP

#include "main.h"
#include <Windows.h>
#include <fstream>
#include <format>

#include "Memory/Hooking.h"
#include "Logger.h"
#include "../vendor/minhook-1.3.3/include/MinHook.h"
#include "../vendor/scripthook/include/main.h"

#include <iostream>

#include "../vendor/scripthook/include/keyboard.h"
#include "../vendor/scripthook/include/natives.h"
#include "../vendor/scripthook/include/types.h"
#include "../vendor/scripthook/include/enums.h"
#include <boost/stacktrace/stacktrace.hpp>

#include <d3d11.h>

#include "ComponentMgr.h"

// #include <boost/program_options/option.hpp>
typedef bool(*SettingMgr__Save)();
typedef bool(*SettingMgr__BeginSave)(uintptr_t a1);

typedef int(*WriteDebugStateToFile)(const WCHAR* fileName);
typedef int(*WriteDebugState)();

typedef __int64 (*UpdateCamFrame)(intptr_t* frame, __int64 a2, __int64 a3);

SettingMgr__Save gimpl_SettingMgr__Save;
WriteDebugStateToFile gimpl_WriteDebugStateToFile;
WriteDebugStateToFile gimpl_WriteDebugState;

uintptr_t save;
uintptr_t beginSave;
uintptr_t beginSave_setting_64;
uintptr_t beginSave_settingDump;

uintptr_t writeDebugStateToFile;
uintptr_t writeDebugState;

// Looks like a common structure
uintptr_t numFramesRendered;
uintptr_t isGamePaused;
uintptr_t isDebugPaused;
uintptr_t isPausedUnk1;
uintptr_t isPausedUnk2;

uintptr_t numStreamingRequests;

bool IsGamePaused()
{
	return *(bool*)isDebugPaused ||
		*(bool*)isGamePaused ||
		*(bool*)isPausedUnk1 ||
		*(bool*)isPausedUnk2;
}

enum GameState
{
	SystemInit = 0,
	GameInit = 1,
	GameRunning = 2,
	GameShutdown = 3,
	SystemShutdown = 4,
};

struct CApp
{
	int8_t _gap0[0x10];
	int8_t _gameState;

	GameState GetGameState()
	{
		return (GameState)_gameState;
	}

	std::string GetGameStateStr()
	{
		// TODO: System init is not really possible to get,
		// because game returns it when CGame pointer is set to null,
		// meaning this class instance wont exist
		switch (_gameState)
		{
		case 0: return "System Init";
		case 1: return "Game Init";
		case 2: return "Game Running";
		case 3: return "Game ShutDown";
		case 4: return "System Shutdown";
		}
		return "Unknown";
	}
};

bool __fastcall aimpl_SettingMgr__BeginSave(uintptr_t a1)
{
	g_logger->Log(std::format("SettingMgr::Save({:x})", a1));

	int v1; // edx

	v1 = 203;
	if (*(unsigned __int16*)(a1 + 64) < 203u) // a1 + 64 seems to be always on 350
		v1 = *(unsigned __int16*)(a1 + 64);
	*(int*)beginSave_setting_64 = v1;
	memmove((void*)beginSave_settingDump, *(const void**)(a1 + 56), 8i64 * v1);
	return gimpl_SettingMgr__Save();
}

float cam_x = 0;
float cam_y = 0;
float cam_z = 0;

UpdateCamFrame gimpl_UpdateCamFrame = NULL;
__int64 __fastcall aimpl_UpdateCamFrame(intptr_t* frame, __int64 a2, __int64 a3)
{
	if (*(bool*)isDebugPaused)
	{
		*(float*)(a2 + 0x40) = cam_x;
		*(float*)(a2 + 0x44) = cam_y;
		*(float*)(a2 + 0x48) = cam_z;
	}

	return gimpl_UpdateCamFrame(frame, a2, a3);
}


struct CPools
{
	int64_t qword0;
	int8_t* pbyte8;
	int32_t MaxPeds;
	int int14;
	int8_t gap18[8];
	int32_t dword20;
};

enum PoolType
{
	POOL_PED = 128,
};

struct camFrame
{
	float FrontX;
	float FrontY;
	float FrontZ;
	float FrontW;
	float UpX;
	float UpY;
	float UpZ;
	float UpW;
	int8_t gap0[32];
	int32_t PositionX;
	int32_t PositionY;
	int32_t PositionZ;
	int32_t PositionW;
	int32_t dword50;
	int32_t dword54;
	int32_t dword58;
	int32_t dword5C;
	int64_t qword60;
	int64_t qword68;
	float float70;
	float float74;
	int32_t dword78;
	int32_t dword7C;
	float float80;
	int32_t dword84;
	int32_t dword88;
	int32_t dword8C;
	int32_t dword90;
	int32_t dword94;
	int32_t dword98;
	int32_t dword9C;
	int32_t dwordA0;
	int32_t dwordA4;
	int32_t dwordA8;
	int32_t dwordAC;
	int32_t dwordB0;
	int32_t dwordB4;
	int32_t dwordB8;
	int32_t dwordBC;
	int32_t dwordC0;
	int32_t dwordC4;
	int32_t dwordC8;
	int32_t dwordCC;
	int8_t wordD0;
};

typedef int64_t _QWORD;

typedef __int64 (*GtaThread__RunScript)(
	__int64 a1,
	int a2,
	const void* a3,
	int a4);

intptr_t gtaThread__RunScript;

GtaThread__RunScript gimpl_GtaThread__RunScript;

__int64 aimpl_GtaThread__RunScript(
	__int64 a1,
	int a2,
	const void* a3,
	int a4)
{
	g_logger->Log(std::format("GtaThread__RunScript: {:x}, {}, {}, {}", a1, a2, a3, a4));
	return gimpl_GtaThread__RunScript(a1, a2, a3, a4);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved);

class MovieEntry
{
	char pad_01[176];
	uint _id;
	char pad_02[4];
	uintptr_t _fileName;
	char pad_03[246];
	uint _state;
	char pad_04[36];

public:
	uint GetId()
	{
		return _id;
	}

	const char* GetFileName()
	{
		return (const char*)&_fileName;
	}

	int GetState()
	{
		return _state;
	}
};
static_assert(sizeof(MovieEntry) == 480);

class MovieStore
{
	MovieEntry* _slots;
	short _slotCount;

public:
	MovieEntry* GetSlot(int index)
	{
		return &_slots[index];
	}

	short GetNumSlots()
	{
		return _slotCount;
	}

	// TODO: Hook
	bool IsSlotActive(int index)
	{
		if (index < 0 || index > 50)
			return false;

		if (index > GetNumSlots())
			return false;

		return GetSlot(index)->GetState() == 3;
	}
};

typedef intptr_t(*GetEntityToQueryFromGUID)(int index);

__int64 __fastcall GetEntityFromGUID(int index)
{
	__int64 v1; // r8
	__int64 v2; // rax

	if (index == -1)
		return 0i64;
	v1 = (unsigned int)index / 256;

	auto u0 = *(_QWORD*)(0x273A3D80A60 + 8);
	auto u1 = *(int8_t*)(v1 + u0);
	g_logger->Log(std::format("{}", u0));
	g_logger->Log(std::format("{}", u1));

	if (u1 == (int8_t)index)
	{
		auto unk1 = *(int*)(0x273A3D80A60 + 20); // 16
		auto unk2 = (unsigned int)(v1 * unk1);
		g_logger->Log(std::format("{}", unk2));
		v2 = *(_QWORD*)0x273A3D80A60 + unk2;
	}
	else
	{
		return 0;
	}
	return *(_QWORD*)(v2 + 8);
}

#include "imgui.h"

typedef _QWORD(*PresentImage)();

void Abort()
{
	g_imgui->Destroy();
	g_hook->UnHookAll();
	MH_Uninitialize();
}

bool logOpen = true;
bool menuOpen = true;
PresentImage gimplPresentImage = NULL;
_QWORD aimplPresentImage()
{
	if(menuOpen)
		PAD::DISABLE_ALL_CONTROL_ACTIONS(0);

	if (IsKeyJustUp(VK_SCROLL))
		menuOpen = !menuOpen;

	if (g_imgui->IsInitialized())
	{
		g_imgui->NewFrame();

		ImGui::GetIO().MouseDrawCursor = menuOpen; // Until we have SetCursor hook

		if(menuOpen)
		{
			ImGui::Begin("rageAm", &menuOpen);

			ImGui::Text("Window Handle: %#X", reinterpret_cast<int>(g_gtaWindow->GetHwnd()));
			//ImGui::Checkbox("Debug Pause", reinterpret_cast<bool*>(isDebugPaused));

			if (ImGui::TreeNode("Action Movies"))
			{
				const auto movieStore = reinterpret_cast<MovieStore*>(0x7FF72097CF70);
				for (int i = 0; i < movieStore->GetNumSlots(); i++)
				{
					if (!movieStore->IsSlotActive(i))
						continue;


					const auto entry = movieStore->GetSlot(i);
					const uint movieId = entry->GetId();
					const auto movieFileName = entry->GetFileName();

					ImGui::Text("%lu - %s", movieId, movieFileName);
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Log"))
			{
				for (const std::string& entry : g_logger->GetEntries())
				{
					ImGui::Text("%s", entry.c_str());
				}
				ImGui::TreePop();
			}
			ImGui::End();
		}

		g_imgui->Render();
	}

	return gimplPresentImage();
}

// TODO: Caching
#include <excpt.h>
void Main()
{
	g_logger->Log("Init rageAm", true);

	g_logger->Log(std::format("MH_Initialize: {}", MH_Initialize() == MH_OK));

	g_componentMgr->RegisterComponents();

	auto gPtr_PresentImage = g_hook->FindPattern("PresentImage", "40 55 53 56 57 41 54 41 56 41 57 48 8B EC 48 83 EC 40 48 8B 0D");
	g_hook->SetHook(gPtr_PresentImage, aimplPresentImage, &gimplPresentImage);

	g_imgui->Init(g_gtaWindow->GetHwnd());

	g_logger->Log("Scanning patterns...");

	//save = g_hook->FindPattern("SettingMgr::Save", "48 83 EC 48 48 83 3D");
	//beginSave = g_hook->FindPattern("SettingMgr::BeginSave", "40 53 48 83 EC 20 0F B7 41 40");
	//beginSave_setting_64 = g_hook->FindOffset("SettingMgr::BeginSave_setting64", beginSave + 0x1C);
	//beginSave_settingDump = g_hook->FindOffset("SettingMgr::BeginSave_settingDump", beginSave + 0x27);

	writeDebugStateToFile = g_hook->FindPattern("WriteDebugStateToFile", "48 83 EC 48 48 83 64 24 30 00 83 64 24 28 00 45");
	writeDebugState = g_hook->FindPattern("WriteDebugState", "48 8B C4 48 89 58 08 55 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 90 48 81 EC 80");

	gtaThread__RunScript = g_hook->FindPattern("GtaThread::RunScript", "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 81 EC 30 01 00 00 49");
	g_hook->SetHook((LPVOID)gtaThread__RunScript, aimpl_GtaThread__RunScript, (LPVOID*)&gimpl_GtaThread__RunScript);

	//while(true)
	//{
	//	if(menuOpen)
	//	{
	//		//PAD::DISABLE_CONTROL_ACTION(0, 0, true);
	//		PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
	//	}

	//	WAIT(0);
	//}

	//intptr_t movieStore = *(intptr_t*)0x7FF72097CF70;
	//short movieSlots = *(short*)(0x7FF72097CF78);

	//// TODO: Test boost stacktrace

	//const auto movieStore = (MovieStore*)(0x7FF72097CF70);
	//g_logger->Log(std::format("Movie Slots: {}", movieStore->GetNumSlots()));
	//for (int i = 0; i < movieStore->GetNumSlots(); i++)
	//{
	//	if (!movieStore->IsSlotActive(i))
	//		continue;

	//	const auto entry = movieStore->GetSlot(i);
	//	uint movieId = entry->GetId();
	//	auto movieFileName = entry->GetFileName();

	//	g_logger->Log(std::format(" - Id({}), FileName({})", movieId, movieFileName));
	//}



	//int pHandle = PLAYER::GET_PLAYER_PED(0);
	////auto getEntityToQuertyFromGuid = reinterpret_cast<GetEntityToQueryFromGUID>(0x7FF71F307EF0);
	//g_logger->Log(std::format("Player Entity: {:x}", GetEntityFromGUID(pHandle)));



	//while (true)
	//{
	//	if (IsKeyJustUp(VK_F9))
	//	{
	//		/*auto fs = g_logger->Open();
	//		fs << boost::stacktrace::stacktrace();
	//		fs.close();*/

	//		// boost::stacktrace::safe_dump_to("Hello.dump");
	//		//std::cout << boost::stacktrace::stacktrace();
	//		//auto bs = boost::stacktrace::stacktrace().as_vector();
	//		//for (auto b : bs)
	//		//{
	//		//	g_logger->Log(b.name());
	//		//}

	//		//SCRIPT::REQUEST_SCRIPT("emergencycall");
	//		//SYSTEM::START_NEW_SCRIPT("emergencycall", 512);
	//		//invoke<int>(0xB8BA7F44DF1575E1, "victor", 0x1, 12390, 5235);
	//	}

	//	WAIT(0);
	//}

	//auto cam = g_hook->FindPattern("UpdateCamFrame", "48 89 5C 24 08 57 48 83 EC 20 8B 42 40 F3");
	//g_hook->SetHook((LPVOID)cam, aimpl_UpdateCamFrame, (LPVOID*)&gimpl_UpdateCamFrame);

	//gimpl_SettingMgr__Save = (SettingMgr__Save)save;
	////g_hook->SetHook((LPVOID)beginSave, &aimpl_SettingMgr__BeginSave);

	//gimpl_WriteDebugStateToFile = (WriteDebugStateToFile)writeDebugStateToFile;

	//// mov rax, cs:CApp
	//CApp* game = *(CApp**)g_hook->FindOffset("writeDebugState_CApp", writeDebugState + 0xAB + 0x3);

	//// mov edx, dword ptr cs:numFramesRendered
	//numFramesRendered = g_hook->FindOffset("writeDebugState_currentFrame", writeDebugState + 0x159 + 0x2);

	//// or al, cs:isDebugPaused
	//isDebugPaused = g_hook->FindOffset("writeDebugState_isDebugPaused", writeDebugState + 0x179 + 0x2);
	//isGamePaused = isDebugPaused - 0x1;
	//isPausedUnk1 = isDebugPaused + 0x1;
	//isPausedUnk2 = isDebugPaused + 0x2;

	//// mov edx, cs:numStreamingRequests
	//numStreamingRequests = g_hook->FindOffset("writeDebugState_numStreamingRequests", writeDebugState + 0x18F + 0x2);

	//g_logger->Log(std::format("CApp - {:x}", (intptr_t)game));
	//if (game)
	//	g_logger->Log(std::format("Game State: {}", game->GetGameStateStr()));
	//g_logger->Log(std::format("Frame: {} - Paused: {}", *(int*)numFramesRendered, IsGamePaused() ? "yes" : "no"));
	//g_logger->Log(std::format("Streaming Requests: {}", *(int*)numStreamingRequests));

	//// TODO:
	//// GetLocalizedString
	//// ReadGameVersion
	//// GetPlayerPosition
	//// CTheScripts::GetEntityToModifyFromGUID__Ped
	//// DirectX Math
	return;

	auto crap1 = (unsigned int)(*(intptr_t*)(0x7FF66B5112C0 + 0x20));
	auto crap2 = (unsigned int)(4 * crap1);
	auto crap3 = (unsigned int)(crap2 >> 2);
	int numPeds = (unsigned int)((4 * *(intptr_t*)(0x7FF66B5112C0 + 0x20)) >> 2);

	//g_logger->Log(std::format("Crap1: {:x} Crap2: {:x} Crap3: {} NumPeds: {}", crap1, crap2, crap3, numPeds));

	CPools* pool = *(CPools**)0x7FF66B5112C0;

	auto maxPeds = pool->MaxPeds;
	auto ped_missions = 0;
	auto ped_reused = 0;
	auto ped_reuse_pool = 0;

	auto __pedIndex = pool->pbyte8;
	auto qword0 = pool->qword0;
	auto pedIterator = (unsigned int)maxPeds;

	g_logger->Log(std::format("dword20: {}", pool->dword20));
	g_logger->Log(std::format("numPeds: {}", 4 * pool->dword20));
	g_logger->Log(std::format("numPeds: {}", (4 * pool->dword20) >> 2));
	g_logger->Log(std::format("numPeds: {}", 1073741882 >> 2));
	g_logger->Log(std::format("qword0: {:x}", qword0));


	// From task_commands.cpp
	/*
	 *  CPed::Pool* pool = CPed::GetPool();
		const int maxPeds = pool->GetSize();
		for(int i = 0; i < maxPeds; i++)
		{
			CPed* pPed = pool->GetSlot(i);
			...
		}
	*/

	for (int i = 0; i < pedIterator; i++)
	{
		auto v31 = *(__pedIndex + i) & 0b10000000;//& POOL_PED; // flag check?

		auto pedPtr = qword0 & ~((v31 | -v31) >> 0x3F);
		//g_logger->Log(std::format("{} \t {:x}", v31, pedPtr));
		g_logger->Log(std::format("{} {:x}", *(__pedIndex + i), pedPtr));
		qword0 += pool->int14;
	}

	//do                                        // Foreach Ped
	//{
	//	//g_logger->Log(std::format("{}", ((uint8_t)*__pedIndex) & 0b10000000));

	//	auto v31 = *__pedIndex & 0b10000000; // flag check?

	//	auto pedPtr = qword0 & ~((v31 | -v31) >> 0x3F);
	//	//g_logger->Log(std::format("{} \t {:x}", v31, pedPtr));
	//	g_logger->Log(std::format("{:x}",pedPtr));

	//	qword0 += pool->int14;
	//	++__pedIndex;
	//	--pedIterator;
	//	continue;

	//	if (!pedPtr || (*(int8_t*)((qword0 & ~((v31 | -(__int64)(*__pedIndex & 128)) >> 0x3F)) + 0x1091) & 0x10) != 0)
	//		pedPtr = 0i64;
	//	if (pedPtr)
	//	{
	//		ped_missions += (*(int16_t*)(pedPtr + 218) & 0xFu) - 6 <= 1;
	//		auto v33 = *(int32_t*)(pedPtr + 5224);
	//		ped_reused += (v33 >> 5) & 1;
	//		ped_reuse_pool += (v33 >> 4) & 1;
	//	}
	//	qword0 += pool->int14;
	//	++__pedIndex;
	//	--pedIterator;
	//} while (pedIterator);

	g_logger->Log(std::format("missions: {} reused: {} reuse_pool: {}", ped_missions, ped_reused, ped_reuse_pool));

	gimpl_WriteDebugStateToFile(L"victor.txt");

	// Main loop
	while (true)
	{
		if (IsKeyJustUp(VK_F9))
		{
			*(bool*)isDebugPaused = !*(bool*)isDebugPaused;
			gimpl_WriteDebugStateToFile(L"victor.txt");
		}

		//auto camFrameP = (camFrame*)0x7FF66AF80750;
		//auto camFrameA = (intptr_t)0x7FF66AF80750;

		//Vector3 pos = ENTITY::GET_ENTITY_COORDS(PLAYER::GET_PLAYER_PED(0), true);
		//ENTITY::SET_ENTITY_ALPHA(PLAYER::GET_PLAYER_PED(0), 100, false);

		//// Front
		//float f_x = pos.x + camFrameP->FrontX;// *(float*)(camFrameA + 0) * 5;
		//float f_y = pos.y + camFrameP->FrontY;// *(float*)(camFrameA + 4) * 5;
		//float f_z = pos.z + camFrameP->FrontZ;// *(float*)(camFrameA + 8) * 5;

		//// Up
		//float u_x = pos.x + camFrameP->UpX;// *(float*)(camFrameA + 16) * 1;
		//float u_y = pos.y + camFrameP->UpY;// *(float*)(camFrameA + 20) * 1;
		//float u_z = pos.z + camFrameP->UpZ;// *(float*)(camFrameA + 24) * 1;

		//GRAPHICS::DRAW_LINE(pos.x, pos.y, pos.z, f_x, f_y, f_z, 255, 255, 255, 255);
		//GRAPHICS::DRAW_LINE(pos.x, pos.y, pos.z, u_x, u_y, u_z, 255, 255, 255, 255);

		//if (*(bool*)isDebugPaused)
		//{
		//	float moveVertical = 0.0f;
		//	float moveHorizontal = 0.0f;

		//	moveVertical += IsKeyDown(0x57) ? 1 : 0;
		//	moveVertical -= IsKeyDown(0x53) ? 1 : 0;
		//	moveHorizontal += IsKeyDown(0x41) ? 1 : 0;
		//	moveHorizontal -= IsKeyDown(0x44) ? 1 : 0;

		//	float dt = SYSTEM::TIMESTEP();
		//	if (moveHorizontal != 0.0f || moveVertical != 0.0f)
		//	{
		//		float length = 1 / sqrt(moveVertical * moveVertical + moveHorizontal * moveHorizontal);
		//		moveVertical *= length;
		//		moveHorizontal *= length;

		//		moveVertical *= dt;
		//		moveHorizontal *= dt;

		//		cam_x += moveHorizontal * 50;
		//		cam_y += moveVertical * 50;
		//	}

		//	cam_z += IsKeyDown(VK_SPACE) ? dt * 50 : 0.0f;
		//	cam_z -= IsKeyDown(VK_CONTROL) ? dt * 50 : 0.0f;
		//}

		//if(IsKeyJustUp(VK_SCROLL))
		//{
		//	gimpl_WriteDebugStateToFile(L"victor.txt");
		//}

		WAIT(0);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		Main();
		//scriptRegister(hModule, Main);
		keyboardHandlerRegister(OnKeyboardMessage);
		break;
	case DLL_PROCESS_DETACH:
		Abort();
		//scriptUnregister(hModule);
		keyboardHandlerUnregister(OnKeyboardMessage);
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:break;
	}

	return TRUE;
}
