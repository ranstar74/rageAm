#include "core.h"

#ifdef AM_INTEGRATED

#include "am/integration/memory/address.h"
#include "am/integration/memory/hook.h"
#include "rage/streaming/streamengine.h"
#include "rage/system/tls.h"
#include "rage/framework/streaming/assetstore.h"
#include "rage/streaming/streamingdefs.h"

#include "headers/commands_2699_16.h"
#include "types.h"

namespace
{
	bool								s_Initialized = false;
	scrThreadId							s_ThreadId;
	pVoid								s_Thread = nullptr;
	gmAddress							s_scrThread_UpdateAll;
	rage::ThreadLocal<pVoid>			tl_CurrentThread;
	rage::ThreadLocal<bool>				tl_ThisThreadIsRunningAScript;
	bool*								sm_UpdatingThreads;
	thread_local pVoid					tl_OldThread;
	thread_local int					tl_BeginEndStackSize;
#ifdef AM_SCR_ENABLE_DISPATCH
	std::mutex							s_DelegateQueueMutex;
	rageam::List<std::function<void()>> s_DelegateQueue;
#endif
}

#ifdef AM_SCR_ENABLE_DISPATCH
void (*gImpl_scrThread_UpdateAll)(int insnCount);
void aImpl_scrThread_UpdateAll(int insnCount)
{
	gImpl_scrThread_UpdateAll(insnCount);

	// Invoke all pending delegates from script update thread
	std::unique_lock lock(s_DelegateQueueMutex);
	for (auto& fn : s_DelegateQueue)
		fn();
	s_DelegateQueue.Clear();
}
#endif

void rageam::integration::scrInit()
{
	AM_ASSERT(!s_Initialized, "scrInit() -> Already initialized!");

	// We must create scrThread instance to use most of the natives because they access it
	// The simplest way is to use some dummy script
	// This is a pretty hellish looking code but in general idea is:
	// - Locate script in asset store, query load request to streaming manager
	// - We can't wait so force-execute all pending streaming requests
	// - Now script (ysc) is loaded, we can create script thread

	ConstString SCRIPT_NAME = "audiotest";

	using namespace rage;

	// We can't call REQUEST_SCRIPT native because it's supposed to be called ONLY FROM SCRIPT,
	// but we are not in the script yet. We have to do streaming request manually
	struct Dummy {};
	auto streamingScripts = reinterpret_cast<fwAssetStore<Dummy, fwAssetDef<Dummy>>*>(strStreamingModuleMgr::GetModule("ysc"));
	strLocalIndex localSlot = streamingScripts->FindSlotFromHashKey(atStringHash(SCRIPT_NAME));
	AM_ASSERT(localSlot != rage::INVALID_STR_INDEX, "scrInit() -> Script '%s' not found!", SCRIPT_NAME);

	strIndex globalSlot = streamingScripts->GetGlobalIndex(localSlot);

	// Add streaming request to load script
	auto requestObject = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"E9 6E 12 00 00", "rage::strStreamingInfoManager::RequestObject+0x50")
		.GetAt(-0x50)
#else
		"40 53 48 83 EC 20 8B 41 14 48 8B D9 48", "rage::strStreamingInfoManager::RequestObject")
		.GetAt(0x33)
		.GetCall()
#endif
		.ToFunc<bool(pVoid, strIndex, u16)>();

	auto strInfo = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"48 8D 05 ?? ?? ?? ?? 48 05 20 02 00 00 48 8B 94 24 80 00 00 00")
#else
		"48 8D 0D ?? ?? ?? ?? 41 B8 01 00 00 00 03 D3 E8 ?? ?? ?? ?? 33 C9")
#endif
		.GetRef(3);

	requestObject(strInfo, globalSlot, 1); // STRFLAG_DONTDELETE = 1 << 0

	// Force load our request
	auto loadAllRequestedObjects = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"88 4C 24 08 48 83 EC 38 E8 ?? ?? ?? ?? 48 89 44 24 28 48 83", "CStreaming::LoadAllRequestedObjects")
#else
		"E8 ?? ?? ?? ?? EB DD B0 01", "CStreaming::LoadAllRequestedObjects")
		.GetCall()
#endif
		.ToFunc<void(bool)>();
	loadAllRequestedObjects(false);
	AM_ASSERT(streamingScripts->GetPtr(localSlot) != nullptr, "scrInit() -> Failed to load script '%s'!", SCRIPT_NAME);

	// Create script thread
	auto getThread = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"89 4C 24 08 48 83 EC 28 83 7C 24 30 00 75 07",
#elif APP_BUILD_2699_16_RELEASE
		"33 D2 44 8B C9 85 C9 74",
#else
		"33 D2 44 8B C1 85",
#endif
		"rage::scrThread::GetThread").ToFunc<pVoid(scrThreadId)>();
	s_ThreadId = scrStartNewScript(SCRIPT_NAME);
	s_Thread = getThread(s_ThreadId);
	AM_ASSERT(s_Thread != nullptr, "scrInit() -> Failed to create thread.");

	// Set state to halted, it won't let game to call abort
#if APP_BUILD_2699_16_RELEASE_NO_OPT | APP_BUILD_2699_16_RELEASE | APP_BUILD_2699_16
	int stateOffset = 32;
#else
	// Was changed in 2802, most likely field shuffling
	int stateOffset = 16;
#endif
	*((char*)s_Thread + stateOffset) = 3; // scrThread::State::HALTED

	// Script is now referenced by created thread, let streaming manager to handle it
	auto clearRequredFlag = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"44 89 44 24 18 89 54 24 10 48 89 4C 24 08 48 83 EC 38 33 C0 85 C0 75 FA 33 C0 85 C0 75 FA 8B",
#elif APP_BUILD_2699_16_RELEASE
		"89 54 24 10 48 83 EC 28 3B 51 08 72 01 CC 44",
#else
		"89 54 24 10 48 83 EC 28 4C 8B C9",
#endif
		"rage::strStreamingInfoManager::ClearRequiredFlag")
		.ToFunc<bool(pVoid, strIndex, u16)>();
	clearRequredFlag(strInfo, globalSlot, 1); // STRFLAG_DONTDELETE = 1 << 0

	// Scan TLS offsets in scrThread::Run
#if APP_BUILD_2699_16_RELEASE_NO_OPT
	gmAddress tlValues = gmAddress::Scan("B8 38 1A 00 00 8B C0 8B 0D ?? ?? ?? ?? 65 48 8B 14 25 58 00 00 00 48 8B 0C CA 8A 54 24 60");
	tl_CurrentThread.SetOffset(*tlValues.GetAt(-82 + 1).To<int*>());
	tl_ThisThreadIsRunningAScript.SetOffset(*tlValues.GetAt(1).To<int*>());
#elif APP_BUILD_2699_16_RELEASE
	gmAddress tlValues = gmAddress::Scan("41 BD 38 1A 00 00");
	// mov r13d, 1A38h
	// mov rdx, [rax + rbp * 8]
	// mov r12d, 1A40h
	tl_CurrentThread.SetOffset(*tlValues.GetAt(2).To<int*>());
	tl_ThisThreadIsRunningAScript.SetOffset(*tlValues.GetAt(6 + 4 + 2).To<int*>());
#else
	gmAddress tlValues = gmAddress::Scan("41 BF 48 08 00 00", "rage::scrThread::Run+0x52");
	// mov r15d, 848h
	// mov r14d, 850h
	tl_CurrentThread.SetOffset(*tlValues.GetAt(2).To<int*>());
	tl_ThisThreadIsRunningAScript.SetOffset(*tlValues.GetAt(6 + 2).To<int*>());
#endif
	sm_UpdatingThreads = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"0F B6 05 ?? ?? ?? ?? 85 C0 74 19 E8 ?? ?? ?? ?? 48 89 44 24 20", "CTheScripts::GetCurrentScriptName+0x19")
		.GetRef(3)
#else
		"40 53 48 83 EC 30 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 0D", "CTheScripts::InternalProcess")
		.GetAt(0xC0)
		.GetRef(2)
		.GetAt(1)
#endif
		.To<bool*>();

#ifdef AM_SCR_ENABLE_DISPATCH
	s_scrThread_UpdateAll = gmAddress::Scan("89 4C 24 08 48 81 EC 98 00 00 00 C6 44 24 46", "rage::scrThread::UpdateAll");
	Hook::Create(s_scrThread_UpdateAll, aImpl_scrThread_UpdateAll, &gImpl_scrThread_UpdateAll);
#endif

	s_Initialized = true;
}

void rageam::integration::scrShutdown()
{
	if (!s_Initialized)
		return;

	scrTerminateThread(s_ThreadId);
	s_ThreadId.Set(0);
	s_Thread = nullptr;

#ifdef AM_SCR_ENABLE_DISPATCH
	{
		std::unique_lock lock(s_DelegateQueueMutex);
		Hook::Remove(s_scrThread_UpdateAll);
		s_DelegateQueue.Destroy();
	}
#endif

	s_Initialized = false;
}

rageam::integration::scrSignature rageam::integration::scrLookupHandler(u64 hashcode)
{
#if APP_BUILD_2699_16_RELEASE_NO_OPT
	static auto	addr_scrCommandHash_Lookup = gmAddress::Scan(
		"0F 83 C5 03 00 00 8B 44 24 30", "rage::scrCommandHash::Lookup").GetAt(-0x3C5);
#else
	static auto	addr_scrCommandHash_Lookup = gmAddress::Scan(
		"48 89 5C 24 18 48 89 7C 24 20 0F", "rage::scrCommandHash::Lookup");
#endif
	static auto scrCommandHash_Lookup = addr_scrCommandHash_Lookup.ToFunc<scrSignature(pVoid, u64)>();

	static auto scrCommandHash = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 89 44 24 58 48 83 7C 24 58 00 75 4D"
#elif APP_BUILD_2699_16_RELEASE
		"48 8D 0D ?? ?? ?? ?? 48 8B 14 EA E8"
#else
		"48 8D 0D ?? ?? ?? ?? 48 8B 14 FA"
#endif
		, "s_CommandHash").GetRef(3);

	scrSignature ppHandler = scrCommandHash_Lookup(scrCommandHash, hashcode);
	AM_ASSERT(ppHandler, "scrLookupHandler() -> Native with hash '%llX' does not exist!", hashcode);
	return ppHandler;
}

#ifdef AM_SCR_ENABLE_DISPATCH
void scrDispatch(const std::function<void()>& fn)
{
	std::unique_lock lock(s_DelegateQueueMutex);
	s_DelegateQueue.Add(fn);
}
#endif

void scrBegin()
{
	AM_ASSERT(s_Initialized, "scrBegin() -> scrInit was not called!");

	tl_OldThread = tl_CurrentThread;
	tl_CurrentThread = s_Thread;
	tl_BeginEndStackSize++;
	tl_ThisThreadIsRunningAScript = true;
	*sm_UpdatingThreads = true;
}

void scrEnd()
{
	AM_ASSERT(tl_CurrentThread, "scrEnd() -> scrBegin was not called!");
	AM_ASSERT(tl_BeginEndStackSize > 0, "scrEnd() -> was called too many times!");

	bool isStackEmpty = tl_OldThread == nullptr;
	tl_CurrentThread = tl_OldThread;
	tl_ThisThreadIsRunningAScript = !isStackEmpty;
	*sm_UpdatingThreads = !isStackEmpty;
	tl_OldThread = nullptr;
	tl_BeginEndStackSize--;
}

#endif
