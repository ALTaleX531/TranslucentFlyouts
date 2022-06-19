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
	PopupMenu = 0b1,
	Tooltip = 0b10,
	ViewControl = 0b100,
	FileExplorer = 0b1000
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

TFLAPI FLOAT WINAPI GetVersionInfo();
TFLAPI BOOL WINAPI IsHookInstalled();
TFLAPI BOOL WINAPI ClearFlyoutConfig();

#ifdef __cplusplus
}
#endif