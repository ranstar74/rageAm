#include "win32.h"

#include <psapi.h>
#include <Shlobj.h>
#include "am/string/string.h"

LARGE_INTEGER ToLargeInteger(LONG64 value)
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	LARGE_INTEGER largeInt{};
	largeInt.QuadPart = value;
	return largeInt;
}

WORD SetConsoleColor(WORD value)
{
	WORD wOldColorAttrs;

	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;

	GetConsoleScreenBufferInfo(handle, &csbiInfo);
	wOldColorAttrs = csbiInfo.wAttributes;

	SetConsoleTextAttribute(handle, value);

	return wOldColorAttrs;
}

void GetModuleNameFromAddress(u64 address, char* buffer, u32 bufferSize)
{
	// NOTE: GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT is very important!
	// Otherwise number of refs to module will be raised to oblivion and we won't be able to unload it.

	HMODULE hModule;
	GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCWSTR)address,
		&hModule);

	GetModuleFileNameA(hModule, buffer, MAX_PATH);

	char* name = strrchr(buffer, '\\') + 1;
	memmove_s(buffer, bufferSize, name, strlen(name) + 1);
}

bool CompressDirectory(const wchar_t* name)
{
	HANDLE hFile = CreateFileW(name,
		GENERIC_READ | GENERIC_WRITE,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);

	DWORD unused;
	USHORT format = COMPRESSION_FORMAT_DEFAULT;
	BOOL result = DeviceIoControl(
		hFile, FSCTL_SET_COMPRESSION, &format,
		sizeof(USHORT), NULL, 0, &unused, NULL);

	CloseHandle(hFile);

	return result;
}

void GetModuleBaseAndSize(DWORD64* lpBase, DWORD64* lpSize)
{
	MODULEINFO moduleInfo = {};

	HMODULE module = GetModuleHandleA(NULL);
	GetModuleInformation(GetCurrentProcess(), module, &moduleInfo, sizeof MODULEINFO);

	if (lpBase) *lpBase = DWORD64(moduleInfo.lpBaseOfDll);
	if (lpSize) *lpSize = DWORD64(moduleInfo.SizeOfImage);
}

void GetDisplayTypeName(wchar_t* dest, u32 destSize, const wchar_t* path, u32 attributes)
{
	const wchar_t* typeName;

	SHFILEINFOW fileInfo{};
	UINT sizeFile = sizeof(fileInfo);
	UINT Flags = SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES;
	SHGetFileInfoW(path, attributes, &fileInfo, sizeFile, Flags);
	typeName = fileInfo.szTypeName;

	String::Copy(dest, destSize, typeName);
}

HMODULE GetCurrentModule()
{
	HMODULE module;
	if (!GetModuleHandleExA(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&GetCurrentModule,
		&module))
	{
		return nullptr;
	}
	return module;
}

bool CanAccessPath(const wchar_t* path, DWORD accessRights)
{
	// Shamelessly stolen from https://stackoverflow.com/questions/7789130/winapi-way-to-determine-if-a-file-is-accessible-private

	bool bRet = false;
	DWORD length = 0;

	if (!GetFileSecurityW(path, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, NULL, 0, &length) &&
		ERROR_INSUFFICIENT_BUFFER == GetLastError())
	{
		PSECURITY_DESCRIPTOR security = operator new(length);

		if (security && GetFileSecurityW(path, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, security, length, &length))
		{
			HANDLE hToken = NULL;

			if (OpenProcessToken(::GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE | STANDARD_RIGHTS_READ, &hToken))
			{
				HANDLE hImpersonatedToken = NULL;

				if (DuplicateToken(hToken, SecurityImpersonation, &hImpersonatedToken))
				{
					GENERIC_MAPPING mapping = { 0xFFFFFFFF };
					PRIVILEGE_SET privileges = { 0 };
					DWORD grantedAccess = 0, privilegesLength = sizeof(privileges);
					BOOL result = FALSE;

					mapping.GenericRead = FILE_GENERIC_READ;
					mapping.GenericWrite = FILE_GENERIC_WRITE;
					mapping.GenericExecute = FILE_GENERIC_EXECUTE;
					mapping.GenericAll = FILE_ALL_ACCESS;

					MapGenericMask(&accessRights, &mapping);

					if (AccessCheck(security, hImpersonatedToken, accessRights,
						&mapping, &privileges, &privilegesLength, &grantedAccess, &result))
					{
						bRet = result == TRUE;
					}

					CloseHandle(hImpersonatedToken);
				}

				CloseHandle(hToken);
			}

			operator delete(security);
		}
	}

	return bRet;
}

void GetSystemDisk(wchar_t* buffer)
{
	wchar_t systemDrive[MAX_PATH];
	SHGetSpecialFolderPathW(NULL, systemDrive, CSIDL_APPDATA, FALSE);
	size_t offset = wcschr(systemDrive, '\\') - systemDrive;
	systemDrive[offset + 1] = L'\0';

	String::Copy(buffer, 4, systemDrive);
}
