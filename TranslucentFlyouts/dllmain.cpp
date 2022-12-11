// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "TranslucentFlyoutsLib.h"

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