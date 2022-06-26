#pragma once
#include <windows.h>

#if TRANSLUCENTFLYOUTS_EXPORTS
	#define TFLAPI __declspec(dllexport)
#else
	#define TFLAPI __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum FlyoutPolicy
{
	Null,
	PopupMenu = 1,
	Tooltip = 1 << 1,
	ViewControl = 1 << 2,
};

TFLAPI BOOL WINAPI RegisterHook();
TFLAPI BOOL WINAPI UnregisterHook();

TFLAPI DWORD WINAPI GetCurrentFlyoutOpacity();
TFLAPI DWORD WINAPI GetCurrentFlyoutEffect();
TFLAPI DWORD WINAPI GetCurrentFlyoutBorder();
TFLAPI DWORD WINAPI GetCurrentFlyoutColorizeOption();
TFLAPI DWORD WINAPI GetCurrentFlyoutPolicy();

TFLAPI DWORD WINAPI GetDefaultFlyoutOpacity();
TFLAPI DWORD WINAPI GetDefaultFlyoutEffect();
TFLAPI DWORD WINAPI GetDefaultFlyoutBorder();
TFLAPI DWORD WINAPI GetDefaultFlyoutColorizeOption();
TFLAPI DWORD WINAPI GetDefaultFlyoutPolicy();

TFLAPI BOOL WINAPI SetFlyoutOpacity(DWORD dwOpacity);
TFLAPI BOOL WINAPI SetFlyoutEffect(DWORD dwEffect);
TFLAPI BOOL WINAPI SetFlyoutBorder(DWORD dwBorder);
TFLAPI BOOL WINAPI SetFlyoutColorizeOption(DWORD dwColorizeOption);
TFLAPI BOOL WINAPI SetFlyoutPolicy(DWORD dwPolicy);

TFLAPI BOOL WINAPI IsHookInstalled();
TFLAPI BOOL WINAPI ClearFlyoutConfig();
TFLAPI VOID WINAPI GetVersionString(LPWSTR pszClassName, const int cchClassName);
TFLAPI VOID WINAPI FlushSettingsCache();

#ifdef __cplusplus
}
#endif