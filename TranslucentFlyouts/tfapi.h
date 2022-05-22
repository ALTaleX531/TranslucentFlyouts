#pragma once
#include <windows.h>

#if TRANSLUCENTFLYOUTS_EXPORTS
	#define TFAPI __declspec(dllexport)
#else
	#define TFAPI __declspec(dllimport)
#endif

extern "C"
{
	TFAPI BOOL WINAPI RegisterHook();
	TFAPI BOOL WINAPI UnregisterHook();

	TFAPI DWORD WINAPI GetCurrentFlyoutOpacity();
	TFAPI DWORD WINAPI GetCurrentFlyoutEffect();
	TFAPI DWORD WINAPI GetCurrentFlyoutBorder();
	TFAPI DWORD WINAPI GetCurrentFlyoutColorizeOption();

	TFAPI DWORD WINAPI GetDefaultFlyoutOpacity();
	TFAPI DWORD WINAPI GetDefaultFlyoutEffect();
	TFAPI DWORD WINAPI GetDefaultFlyoutBorder();
	TFAPI DWORD WINAPI GetDefaultFlyoutColorizeOption();

	TFAPI BOOL WINAPI SetFlyoutOpacity(DWORD dwOpacity);
	TFAPI BOOL WINAPI SetFlyoutEffect(DWORD dwEffect);
	TFAPI BOOL WINAPI SetFlyoutBorder(DWORD dwBorder);
	TFAPI BOOL WINAPI SetFlyoutColorizeOption(DWORD dwColorizeOption);

	TFAPI FLOAT WINAPI GetVersionInfo();
	TFAPI BOOL WINAPI IsHookInstalled();
}