#pragma once
#include "ApiEx.hpp"
#include "cpprt.h"
#include "framework.h"
#undef StartService

namespace TranslucentFlyouts::Application
{
#ifdef _WIN64
	constexpr std::wstring_view g_serviceName{ L"Local\\TranslucentFlyouts.Win32.x64" };
#else
	constexpr std::wstring_view g_serviceName{ L"Local\\TranslucentFlyouts.Win32.x86" };
#endif
	// Global hook.
	HRESULT InstallHook();
	HRESULT UninstallHook();
	// Service
	extern "C" bool IsServiceRunning();
	HRESULT StopService();
	HRESULT KillService();
	HRESULT StartService(HWND hWnd = nullptr);
	// Installer.
	HRESULT InstallApp();
	HRESULT UninstallApp();

	void ClearCache();
}