#pragma once
#include "pch.h"
#include "Win32HookHelper.h"
#include "WRHookHelper.h"
#include "DetoursHelper.h"

namespace TranslucentFlyoutsLib
{
	extern void Startup();
	extern void Shutdown();

	extern void OnWindowsCreated(HWND hWnd);
	extern void OnWindowsDestroyed(HWND hWnd);
	extern void OnWindowShowed(HWND hWnd);

	extern void CALLBACK HandleWinEvent(
	    HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
	    LONG idObject, LONG idChild,
	    DWORD dwEventThread, DWORD dwmsEventTime
	);
};