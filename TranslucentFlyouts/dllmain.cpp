// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "tfapi.h"
#include "TranslucentFlyoutsLib.h"
#ifdef _WIN64
	#pragma comment(lib, "..\\Libraries\\x64\\libkcrt.lib")
	#pragma comment(lib, "..\\Libraries\\x64\\ntdll.lib")
#else
	#pragma comment(lib, "..\\Libraries\\x86\\libkcrt.lib")
	#pragma comment(lib, "..\\Libraries\\x86\\ntdll.lib")
#endif // _WIN64

#pragma data_seg("shared")
HHOOK		g_hHook = nullptr;
#pragma data_seg()
#pragma comment(linker,"/SECTION:shared,RWS")

HMODULE		g_hModule = nullptr;

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

	BOOL WINAPI RegisterHook()
	{
		if (!g_hHook)
		{
			g_hHook = SetWindowsHookEx(
			              WH_CALLWNDPROC, HookProc, g_hModule, 0
			          );
			SendNotifyMessage(HWND_BROADCAST, WM_NULL, 0, 0);
		}
		return g_hHook != 0;
	}

	BOOL WINAPI UnregisterHook()
	{
		BOOL bResult = FALSE;
		if (g_hHook)
		{
			bResult = UnhookWindowsHookEx(g_hHook);
			SendNotifyMessage(HWND_BROADCAST, WM_NULL, 0, 0);
		}
		return bResult;
	}

	DWORD WINAPI GetCurrentFlyoutOpacity()
	{
		DWORD dwSize = sizeof(DWORD);
		DWORD dwOpacity = GetDefaultFlyoutOpacity();
		RegGetValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), TEXT("FlyoutOpacity"), RRF_RT_REG_DWORD, nullptr, &dwOpacity, &dwSize);
		return dwOpacity;
	}
	DWORD WINAPI GetCurrentFlyoutEffect()
	{
		DWORD dwSize = sizeof(DWORD);
		DWORD dwOpacity = GetDefaultFlyoutEffect();
		RegGetValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), TEXT("FlyoutEffect"), RRF_RT_REG_DWORD, nullptr, &dwOpacity, &dwSize);
		return dwOpacity;
	}

	DWORD WINAPI GetDefaultFlyoutOpacity()
	{
		return 154;
	}
	DWORD WINAPI GetDefaultFlyoutEffect()
	{
		return 4;
	}

	BOOL WINAPI SetFlyoutOpacity(DWORD dwOpacity)
	{
		HKEY hKey = nullptr;
		LRESULT lResult = NO_ERROR;
		lResult = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		lResult = RegSetValueEx(hKey, TEXT("FlyoutOpacity"), 0, REG_DWORD, (LPBYTE)&dwOpacity, sizeof(DWORD));
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		RegCloseKey(hKey);
		return TRUE;
	}

	BOOL WINAPI SetFlyoutEffect(DWORD dwEffect)
	{
		HKEY hKey = nullptr;
		LRESULT lResult = NO_ERROR;
		lResult = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		lResult = RegSetValueEx(hKey, TEXT("FlyoutEffect"), 0, REG_DWORD, (LPBYTE)&dwEffect, sizeof(DWORD));
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		RegCloseKey(hKey);
		return TRUE;
	}

	FLOAT WINAPI GetVersionInfo()
	{
		return 1.0f;
	}

	BOOL WINAPI IsHookInstalled()
	{
		return g_hHook != 0;
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

