#undef UNICODE

#include <cstdio>
#include <Windows.h>
#include <tlHelp32.h>
#include <cstring>
#include <filesystem>
#include <tchar.h>
#include <psapi.h>

const char* exe = "GTA5.exe";
const char* amDllName = "rageAm.dll";
const char* amDllPath = nullptr;
const char* scyllaDllPath = nullptr;
HANDLE hGtaProc = nullptr;

enum eLauncherTasks
{
	TASK_LOAD,
	TASK_UNLOAD,
};

// Calls function in DLL by creating remote thread.
// Returns thread handle.
HANDLE RemoteLibraryFunction_Thread(HANDLE hProc, LPVOID addr, LPVOID pParams)
{
	return CreateRemoteThread(hProc, nullptr, 0,
		(LPTHREAD_START_ROUTINE)addr, pParams, 0, nullptr);
}

// Frees memory for params passed to CreateRemoteThread
void FreeParams(HANDLE hProc, LPVOID pParams)
{
	VirtualFreeEx(hProc, pParams, 0, MEM_RELEASE);
}

// Allocates memory for parameters passed to CreateRemoteThread
LPVOID AllocateParams(HANDLE hProc, SIZE_T paramsSize, LPVOID pParams)
{
	LPVOID mem = VirtualAllocEx(hProc, nullptr, paramsSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!mem)
		return nullptr;

	if (!WriteProcessMemory(hProc, mem, pParams, paramsSize, nullptr))
	{
		FreeParams(hProc, pParams);
		return nullptr;
	}
	return mem;
}

// Gets module handle in the process by path to DLL
HMODULE GetModule(HANDLE hProc, LPCSTR dllPath)
{
	HMODULE hMods[1024];
	DWORD cbNeeded;
	if (EnumProcessModules(hProc, hMods, sizeof hMods, &cbNeeded))
	{
		for (unsigned int i = 0; i < cbNeeded / sizeof HMODULE; i++)
		{
			TCHAR szModName[MAX_PATH];

			if (!GetModuleFileNameEx(hProc, hMods[i], szModName, sizeof szModName / sizeof TCHAR))
				continue;

			if (strcmp(szModName, dllPath) == 0)
				return hMods[i];
		}
	}
	return nullptr;
}

// Calls function in DLL
BOOL RemoteLibraryFunction(HANDLE hProc, LPCSTR moduleFileName, LPCSTR procName, SIZE_T paramsSize = 0, LPVOID pParams = nullptr)
{
	// Adapted from:
	// https://stackoverflow.com/questions/13428881/calling-a-function-in-an-injected-dll

	LPVOID pFunctionAddress = (LPVOID)GetProcAddress(GetModuleHandleA(moduleFileName), procName);
	if (!pFunctionAddress)
	{
		HMODULE hModule = GetModule(hProc, moduleFileName);
		if (!hModule)
		{
			printf("Module %s was not loaded. Unable to call remote function.", moduleFileName);
			return FALSE;
		}

		// Since hModule is base address of DLL we can load DLL here, get procedure address and find offset.
		// Then use base address of DLL loaded in the process with offset we've got before.
		HMODULE refLib = LoadLibraryExA(moduleFileName, nullptr, DONT_RESOLVE_DLL_REFERENCES);
		FARPROC addr = GetProcAddress(refLib, procName);
		uint64_t offset = (uint64_t)addr - (uint64_t)refLib;
		FreeLibrary(refLib);

		pFunctionAddress = (LPVOID)((uint64_t)hModule + offset);
	}

	if (!pFunctionAddress)
	{
		printf("Unable to get remote function address %s!%s\n", moduleFileName, procName);
		return FALSE;
	}

	LPVOID lpRemoteParams = nullptr;
	if (pParams)
	{
		lpRemoteParams = AllocateParams(hProc, paramsSize, pParams);
		if (!lpRemoteParams)
		{
			printf("Unable to allocate memory for parameters.\n");
			return FALSE;
		}
	}

	HANDLE hThread = RemoteLibraryFunction_Thread(hProc, pFunctionAddress, lpRemoteParams);
	if (!hThread)
	{
		printf("Unable to create remote thread for %s!%s\n", moduleFileName, procName);
		return FALSE;
	}

	WaitForSingleObject(hThread, -1);
	CloseHandle(hThread);

	if (lpRemoteParams)
		FreeParams(hProc, pParams);

	return TRUE;
}

// Gets PID by process name
DWORD GetProcID(LPCSTR procName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	PROCESSENTRY32 PE32{};
	PE32.dwSize = sizeof PE32;

	DWORD procID = 0;
	BOOL anyProc = Process32First(hSnapshot, &PE32);
	while (anyProc)
	{
		if (strcmp(PE32.szExeFile, procName) == 0)
		{
			procID = PE32.th32ProcessID;
			break;
		}
		anyProc = Process32Next(hSnapshot, &PE32);
	}
	CloseHandle(hSnapshot);

	return procID;
}

// Opens process with (Read | Write | CreateThread) access
HANDLE OpenProc(DWORD procID)
{
	return OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, procID);
}

// Gets whether DLL is loaded in process or not
BOOL IsModuleLoaded(HANDLE hProc, LPCSTR dllPath)
{
	return GetModule(hProc, dllPath) != nullptr;
}

// Injects DLL into the process
BOOL LoadDLL(HANDLE hProc, LPCSTR dllPath)
{
	if (IsModuleLoaded(hProc, dllPath))
	{
		printf("DLL %s is already loaded.\n", dllPath);
		return false;
	}

	SIZE_T paramsSize = strlen(dllPath);
	LPVOID params = (LPVOID)dllPath;
	HMODULE hModule;
	if (!RemoteLibraryFunction(hProc, "kernel32.dll", "LoadLibraryA", paramsSize, params) ||
		!((hModule = GetModule(hProc, dllPath))))
	{
		printf("Injecting %s failed.\n", dllPath);
		return false;
	}

	printf("Injected %s status OK, hModule: %p\n", dllPath, (LPVOID)hModule);
	return true;
}

// Unloads (ejects) dll from the process
BOOL UnloadDLL(HANDLE hProc, LPCSTR dllPath)
{
	HMODULE hModule = GetModule(hProc, dllPath);
	if (hModule == nullptr)
	{
		printf("DLL %s was not loaded into the process.\n", dllPath);
		return false;
	}

	constexpr int maxAttempts = 100;
	int attempts = 0;
	while (GetModule(hProc, dllPath) != nullptr && attempts++ < maxAttempts)
	{
		CreateRemoteThread(hProc, nullptr, 0,
			(PTHREAD_START_ROUTINE)FreeLibraryAndExitThread, // NOLINT(clang-diagnostic-cast-function-type)
			hModule, 0, nullptr);
	}

	bool unloaded = GetModule(hProc, dllPath) == nullptr;
	if (unloaded && attempts == 1)
		printf("DLL %s was unloaded with status OK\n", dllPath);
	else if (unloaded)
		printf("DLL %s was unloaded with status OK, but took several attempts... %i unreleased refs.\n", dllPath, attempts - 1);
	else
		printf("DLL %s failed to unload, possible deadlock or unreleased module refs.\n", dllPath);
	return unloaded;
}

void InitRageAm()
{
	if (IsModuleLoaded(hGtaProc, amDllPath))
	{
		printf("rageAm already loaded.\n");
		return;
	}

	if (!LoadDLL(hGtaProc, amDllPath))
		return;

	if (RemoteLibraryFunction(hGtaProc, amDllPath, "Init"))
		printf("rageAm loaded.\n");
	else
		printf("rageAm failed to load.\n");
}

void ShutdownRageAm()
{
	if (!hGtaProc)
		return;

	if (!IsModuleLoaded(hGtaProc, amDllPath))
	{
		printf("rageAm not loaded.\n");
		return;
	}

	printf("calling !rageam.dll::Shutdown\n");

	RemoteLibraryFunction(hGtaProc, amDllPath, "Shutdown");

	if (UnloadDLL(hGtaProc, amDllPath))
		printf("rageAm unloaded.\n");
	else
		printf("rageAm failed to unload.\n");
}

// NOTE: Argument order is preserved.
// Arguments:
// -exe Name of executable; default is 'GTA5.exe'
// -dll Path to rageAm DLL; optional
// -scylla Path to 'scyllahide' DLL; optional
// -load Loads dll into the process; optional
// -unload Unloads dll from the process; optional
// -manual Requires pressing any key to execute task (loading / unloading) etc; optional
int main(int argc, char* argv[])
{
	bool manual = false;
	std::vector<eLauncherTasks> tasks;

	for (int i = 0; i < argc; i++)
	{
		const char* arg = argv[i];

		// Skip '-'
		arg++;

		if (strcmp(arg, "exe") == 0)
		{
			exe = argv[++i];
			continue;
		}

		if (strcmp(arg, "dll") == 0)
		{
			amDllPath = argv[++i];
			continue;
		}

		if (strcmp(arg, "scylla") == 0)
		{
			scyllaDllPath = argv[++i];
			continue;
		}

		if (strcmp(arg, "manual") == 0)
		{
			manual = true;
			continue;
		}

		if (strcmp(arg, "load") == 0)
		{
			tasks.push_back(TASK_LOAD);
			continue;
		}

		if (strcmp(arg, "unload") == 0)
		{
			tasks.push_back(TASK_UNLOAD);
			continue;
		}
	}

	if (amDllPath == nullptr)
	{
		printf("DLL path is not specified.\n");
		return -1;
	}

	DWORD procID = GetProcID(exe);
	hGtaProc = OpenProc(procID);

	if (!hGtaProc)
	{
		printf("Unable to open process: %s.", exe);
		return  0; // To satisfy and not interrupt compiler
	}

	printf("PID: %lu\n", procID);

	if (scyllaDllPath)
		LoadDLL(hGtaProc, scyllaDllPath);

	for (eLauncherTasks task : tasks)
	{
		if (manual) (void)getchar();

		if (task == TASK_LOAD) InitRageAm();
		else if (task == TASK_UNLOAD) ShutdownRageAm();
	}
	CloseHandle(hGtaProc);
}
