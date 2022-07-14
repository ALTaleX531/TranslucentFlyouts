#include "pch.h"
#include "tflapi.h"
#include "TranslucentFlyoutsLib.h"

#pragma data_seg("shared")
HWINEVENTHOOK g_hHook = nullptr;
#pragma data_seg()
#pragma comment(linker,"/SECTION:shared,RWS")

using namespace TranslucentFlyoutsLib;
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
			                                EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW,
			                                g_hModule,
			                                TranslucentFlyoutsLib::HandleWinEvent,
			                                0, 0,
			                                WINEVENT_INCONTEXT
			                            )
			              ) != 0 ? TRUE : FALSE
			          );
			SendNotifyMessage(HWND_BROADCAST, WM_NULL, 0, 0);
		}
		else
		{
			SetLastError(ERROR_ALREADY_EXISTS);
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
		else
		{
			SetLastError(ERROR_INVALID_PARAMETER);
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

	DWORD WINAPI GetCurrentFlyoutPolicy()
	{
		DWORD dwSize = sizeof(DWORD);
		DWORD dwColorizeOption = GetDefaultFlyoutPolicy();
		RegGetValue(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), TEXT("FlyoutPolicy"), RRF_RT_REG_DWORD, nullptr, &dwColorizeOption, &dwSize);
		return dwColorizeOption;
	}

	DWORD WINAPI GetDefaultFlyoutOpacity()
	{
		return 154;
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
		return Auto;
	}

	DWORD WINAPI GetDefaultFlyoutPolicy()
	{
		return PopupMenu;
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

	BOOL WINAPI SetFlyoutPolicy(DWORD dwPolicy)
	{
		HKEY hKey = nullptr;
		LRESULT lResult = NO_ERROR;
		lResult = RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("SOFTWARE\\TranslucentFlyouts"), 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, nullptr, &hKey, nullptr);
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		lResult = RegSetValueEx(hKey, TEXT("FlyoutPolicy"), 0, REG_DWORD, (LPBYTE)&dwPolicy, sizeof(DWORD));
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		RegCloseKey(hKey);
		return TRUE;
	}

	BOOL WINAPI IsHookInstalled()
	{
		return g_hHook != nullptr;
	}

	BOOL WINAPI ClearFlyoutConfig()
	{
		HKEY hKey = nullptr;
		LRESULT lResult = NO_ERROR;
		lResult = RegOpenKeyEx(HKEY_CURRENT_USER, nullptr, 0, GENERIC_WRITE | KEY_WOW64_64KEY, &hKey);
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		lResult = RegDeleteTree(hKey, TEXT("SOFTWARE\\TranslucentFlyouts"));
		if (lResult != NO_ERROR)
		{
			return FALSE;
		}
		return TRUE;
	}

	VOID WINAPI GetVersionString(LPWSTR pszClassName, const int cchClassName)
	{
		wcscpy_s(pszClassName, cchClassName, L"1.0.2");
	}

	VOID WINAPI FlushSettingsCache()
	{
		/*TCHAR pszMutexName[] = TEXT("Local\\FlushSettingsCache");
		HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, pszMutexName);
		if (!hMutex)
		{
			hMutex = CreateMutex(nullptr, TRUE, pszMutexName);
		}
		else
		{
			WaitForSingleObject(hMutex, INFINITE);
		}
		if (hMutex)
		{
			g_settings.Update();
			ReleaseMutex(hMutex);
			CloseHandle(hMutex);
		}*/
	}
}