// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "TranslucentFlyoutsLib.h"

HMODULE			g_hModule;
HHOOK			g_hHook;

extern "C"
{
	LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
	{
		PCWPSTRUCT lpCWPStruct = (PCWPSTRUCT)lParam;
		if (nCode == HC_ACTION)
		{
			TranslucentFlyoutsLib::WndProc(
				lpCWPStruct->hwnd,
				lpCWPStruct->message,
				lpCWPStruct->wParam,
				lpCWPStruct->lParam
			);
		}
		return CallNextHookEx(g_hHook, nCode, wParam, lParam);
	}

	__declspec(dllexport) bool RegisterHook()
	{
		g_hHook = SetWindowsHookEx(
			WH_CALLWNDPROC, HookProc, g_hModule, 0
		);
		SendNotifyMessage(HWND_BROADCAST, WM_NULL, 0, 0);
		return g_hHook != 0;
	}

	__declspec(dllexport) bool UnregisterHook()
	{
		BOOL bResult = UnhookWindowsHookEx(g_hHook);
		SendNotifyMessage(HWND_BROADCAST, WM_NULL, 0, 0);
		return bResult;
	}
}

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

