// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "tflapi.h"
#include "TranslucentFlyoutsLib.h"

#ifdef _WIN64
	#pragma comment(lib, "..\\Libraries\\x64\\libkcrt.lib")
	#pragma comment(lib, "..\\Libraries\\x64\\ntdll.lib")
#else
	#pragma comment(lib, "..\\Libraries\\x86\\libkcrt.lib")
	#pragma comment(lib, "..\\Libraries\\x86\\ntdll.lib")
#endif // _WIN64

HMODULE g_hModule = nullptr;

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  dwReason,
    LPVOID lpReserved
)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			g_hModule = hModule;
			DisableThreadLibraryCalls(hModule);
			TranslucentFlyoutsLib::Startup();
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			TranslucentFlyoutsLib::Shutdown();
			break;
		}
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

