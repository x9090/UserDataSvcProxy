#include <windows.h>
#include <strsafe.h>
#include <map>
#include <string>

#pragma comment(lib, "Version.lib") // Require for GetFileVersionInfoSizeW

std::map<UINT64, UINT32> Jet_paramVerPageSizeOffset = { {0xA00004A6104F2LL /*10.0.19041.1266*/, 0x2FE828}, {0xA000055F00001LL /*10.0.22000.1*/, 0x390828} };
std::map<UINT64, UINT32> Jet_paramDatabasePageSize = { {0xA00004A6104F2LL /*10.0.19041.1266*/, 0x2FD438}, {0xA000055F00001LL /*10.0.22000.1*/, 0x38f438} };
std::map<UINT64, UINT32> Jet_SectionSizeOffset = { {0xA00004A6104F2LL /*10.0.19041.1266*/, 0x306B30}, {0xA000055F00001LL /*10.0.22000.1*/, 0x399370} };
std::map<UINT64, UINT32> Jet_GranularityOffset = { {0xA00004A6104F2LL /*10.0.19041.1266*/, 0x306B30}, {0xA000055F00001LL /*10.0.22000.1*/, 0x35ae9} };
std::map<UINT64, UINT32> Jet_ErrOSUSetResourceManagerGlobals = { {0xA00004A6104F2LL /*10.0.19041.1266*/, 0x3e5e9}, {0xA000055F00001LL /*10.0.22000.1*/, 0x35ae9} };

typedef  DWORD64 (__fastcall *SERVICE_MAIN)(int argc, LPCWSTR* argv);
typedef  VOID (__fastcall* SVCHOST_PUSH_SERVICE_GLOBALS)(LPVOID arg1);

SERVICE_MAIN pfnUserDataServiceMain;
SVCHOST_PUSH_SERVICE_GLOBALS pfnSvchostPushServiceGlobals;

UINT64 WINAPI GetModuleVersion(WCHAR* pModulePath)
{
	DWORD				dwHandle;
	DWORD				VersionInfoSize;
	VS_FIXEDFILEINFO* pFixedFileInfo;
	LARGE_INTEGER		ModuleVersion;
	UINT				Size;
	VOID* pVersionData = NULL;

	ModuleVersion.QuadPart = 0;
	VersionInfoSize = GetFileVersionInfoSizeW(pModulePath, &dwHandle);
	pVersionData = malloc(VersionInfoSize);
	if (pVersionData == NULL) goto End;

	if (!GetFileVersionInfoW(pModulePath, 0, VersionInfoSize, pVersionData)) goto End;

	if (!VerQueryValueW(pVersionData, L"\\", (VOID**)&pFixedFileInfo, &Size)) goto End;
	ModuleVersion.HighPart = pFixedFileInfo->dwProductVersionMS;
	ModuleVersion.LowPart = pFixedFileInfo->dwProductVersionLS;
End:
	if (pVersionData != NULL) free(pVersionData);
	return (ModuleVersion.QuadPart);
}


BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved)
{
	return true;
}

extern "C" __declspec(dllexport) VOID WINAPI SvchostPushServiceGlobals(LPVOID arg1) {

	//
	// Get the entry point of the target ServiceMain
	//

	HMODULE UserDataSvc = LoadLibraryA("c:\\windows\\system32\\UserDataService.dll");
	pfnSvchostPushServiceGlobals = (SVCHOST_PUSH_SERVICE_GLOBALS)GetProcAddress(UserDataSvc, "SvchostPushServiceGlobals");

	if (!pfnSvchostPushServiceGlobals) {
		OutputDebugStringA("UserDataService!SvchostPushServiceGlobals loading failed!\n");
		return;
	}

	pfnSvchostPushServiceGlobals(arg1);
}

extern "C" __declspec(dllexport) VOID WINAPI ServiceMain(DWORD dwArgc, LPCWSTR * lpszArgv)
{
	__debugbreak();
	//
	// Get the entry point of the target ServiceMain
	//

	HMODULE UserDataSvc = LoadLibraryA("c:\\windows\\system32\\UserDataService.dll");
	pfnUserDataServiceMain = (SERVICE_MAIN)GetProcAddress(UserDataSvc, "ServiceMain");

	if (!pfnUserDataServiceMain) {
		OutputDebugStringA("UserDataService!ServceMain loading failed!\n");
		return;
	}

	//
	// Get the target module that we want to patch in runtime
	//

	const WCHAR *TargetModule = L"c:\\windows\\system32\\ESENT.dll";
	HMODULE EsentMod = GetModuleHandleW(TargetModule);

	if (!EsentMod) {
		OutputDebugStringA("Target module \"ESENT.dll\" not loaded yet!\n");
		return;
	}

	UINT64 RuntimeVersion = GetModuleVersion((WCHAR*)TargetModule);

	if (!Jet_paramVerPageSizeOffset.contains(RuntimeVersion)) {
		char msg[256] = { 0 };
		StringCchPrintfA(msg, 256, "ESENT.dll version 0x%I64x not found!\n", RuntimeVersion);
		OutputDebugStringA(msg);
		__debugbreak();
		return;
	}

	size_t cb;
	DWORD64 DefaultMinVerPageSize = 0;
	DWORD64 DefaultMaxVerPageSize = 0;
	DWORD32 offset = Jet_paramVerPageSizeOffset.at(RuntimeVersion);
	ReadProcessMemory(GetCurrentProcess(), (LPVOID)((BYTE*)EsentMod + offset + 0x40), &DefaultMinVerPageSize, sizeof DWORD64, &cb);
	ReadProcessMemory(GetCurrentProcess(), (LPVOID)((BYTE*)EsentMod + offset + 0x48), &DefaultMaxVerPageSize, sizeof DWORD64, &cb);

	if (DefaultMinVerPageSize != 0x2000) {
		OutputDebugStringA("Seems to be invalid default minimum version page size\n");
		__debugbreak();
		return;
	}

	if (DefaultMaxVerPageSize != 0x8000) {
		OutputDebugStringA("Seems to be invalid default maximum version page size\n");
		__debugbreak();
		return;
	}

	DWORD64 NewMinVerPageSize = 0x10000;
	DWORD64 NewMaxVerPageSize = 0x10000;
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)((BYTE*)EsentMod + offset + 0x40), &NewMinVerPageSize, sizeof DWORD64, &cb);
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)((BYTE*)EsentMod + offset + 0x48), &NewMaxVerPageSize, sizeof DWORD64, &cb);
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)((BYTE*)EsentMod + offset + 0x50), &NewMaxVerPageSize, sizeof DWORD64, &cb);

	//DWORD DefaultMaxDatabasePageSize = 0;
	//offset = Jet_ErrOSUSetResourceManagerGlobals.at(RuntimeVersion);
	//ReadProcessMemory(GetCurrentProcess(), (LPVOID)((BYTE*)EsentMod + offset), &DefaultMaxDatabasePageSize, sizeof DWORD, &cb);

	//if (DefaultMaxDatabasePageSize != 0x10000) {
	//	OutputDebugStringA("Seems to be invalid default maximum database page size\n");
	//	__debugbreak();
	//	return;
	//}

	//WriteProcessMemory(GetCurrentProcess(), (LPVOID)((BYTE*)EsentMod + offset), &NewMinVerPageSize, sizeof DWORD, &cb);

	/*DWORD DefaultSectionSize = 0;
	offset = Jet_SectionSizeOffset.at(RuntimeVersion);
	ReadProcessMemory(GetCurrentProcess(), (LPVOID)((BYTE*)EsentMod + offset), &DefaultSectionSize, sizeof DWORD, &cb);

	if (DefaultSectionSize != 0x10000) {
		OutputDebugStringA("Seems to be invalid default section size\n");
		__debugbreak();
		return;
	}

	WriteProcessMemory(GetCurrentProcess(), (LPVOID)((BYTE*)EsentMod + offset), &NewMinVerPageSize, sizeof DWORD, &cb);*/

	pfnUserDataServiceMain(dwArgc, lpszArgv);

}