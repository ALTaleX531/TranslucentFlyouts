#include "pch.h"
#include "tflapi.h"
#include "TranslucentFlyoutsLib.h"

#pragma data_seg("shared")
HWINEVENTHOOK g_hHook = nullptr;
#pragma data_seg()
#pragma comment(linker,"/SECTION:shared,RWS")

extern HMODULE g_hModule;

extern "C"
{
	BOOL WINAPI RegisterHook()
	{
		BOOL bResult = FALSE;
		if (!IsHookInstalled())
		{
			bResult = (
			              (
			                  g_hHook = SetWinEventHook(
			                                EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY,
			                                g_hModule,
			                                TranslucentFlyoutsLib::HandleWinEvent,
			                                0, 0,
			                                WINEVENT_INCONTEXT
			                            )
			              ) != 0 ? TRUE : FALSE
			          );
			SendNotifyMessage(HWND_BROADCAST, WM_NULL, 0, 0);

		}
		return bResult;
	}

	BOOL WINAPI UnregisterHook()
	{
		BOOL bResult = FALSE;
		if (IsHookInstalled())
		{
			bResult = UnhookWinEvent(g_hHook);
			g_hHook = nullptr;
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

	DWORD WINAPI GetCurrentFlyoutBorder()
	{
		DWORD dwSize = sizeof(DWORD);
		DWORD dwBorder = GetDefaultFlyoutBorder();
		RegGetValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), TEXT("FlyoutBorder"), RRF_RT_REG_DWORD, nullptr, &dwBorder, &dwSize);
		return dwBorder;
	}

	DWORD WINAPI GetCurrentFlyoutColorizeOption()
	{
		DWORD dwSize = sizeof(DWORD);
		DWORD dwColorizeOption = GetDefaultFlyoutColorizeOption();
		RegGetValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), TEXT("FlyoutColorizeOption"), RRF_RT_REG_DWORD, nullptr, &dwColorizeOption, &dwSize);
		return dwColorizeOption;
	}

	DWORD WINAPI GetDefaultFlyoutOpacity()
	{
		return 102;
	}

	DWORD WINAPI GetDefaultFlyoutEffect()
	{
		return 4;
	}

	DWORD WINAPI GetDefaultFlyoutBorder()
	{
		return 0;
	}

	DWORD WINAPI GetDefaultFlyoutColorizeOption()
	{
		return 0;
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

	BOOL WINAPI SetFlyoutBorder(DWORD dwBorder)
	{
		HKEY hKey = nullptr;
		LRESULT lResult = NO_ERROR;
		lResult = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		lResult = RegSetValueEx(hKey, TEXT("FlyoutBorder"), 0, REG_DWORD, (LPBYTE)&dwBorder, sizeof(DWORD));
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		RegCloseKey(hKey);
		return TRUE;
	}

	BOOL WINAPI SetFlyoutColorizeOption(DWORD dwColorizeOption)
	{
		HKEY hKey = nullptr;
		LRESULT lResult = NO_ERROR;
		lResult = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		lResult = RegSetValueEx(hKey, TEXT("FlyoutColorizeOption"), 0, REG_DWORD, (LPBYTE)&dwColorizeOption, sizeof(DWORD));
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

	BOOL WINAPI ClearFlyoutConfig()
	{
		HKEY hKey = nullptr;
		LRESULT lResult = NO_ERROR;
		lResult = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), 0, KEY_SET_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY, &hKey);
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		lResult = RegDeleteTree(hKey, nullptr);
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		return TRUE;
	}
}