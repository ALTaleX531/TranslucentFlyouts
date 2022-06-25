#pragma once
#include "pch.h"
#include "HookHelper.h"
#include "DetoursHelper.h"

namespace TranslucentFlyoutsLib
{
	extern void Startup();
	extern void Shutdown();

	extern void OnWindowsCreated(HWND hWnd);
	extern void OnWindowsDestroyed(HWND hWnd);
	extern void OnWindowShowed(HWND hWnd);
};