#pragma once
#include "Api.hpp"
#include "cpprt.h"
#include "framework.h"
#undef StartService
#ifdef TFMODERN_EXPORTS
#define TFMODERN_API __declspec(dllexport)
#else
#define TFMODERN_API __declspec(dllimport)
#endif

namespace TranslucentFlyouts::Application
{
#ifdef _WIN64
	constexpr std::wstring_view g_serviceName{ L"Local\\TranslucentFlyouts.Immersive.x64" };
#else
	constexpr std::wstring_view g_serviceName{ L"Local\\TranslucentFlyouts.Immersive.x86" };
#endif
	// Global hook.
	HRESULT InstallHook();
	HRESULT UninstallHook();
	// Service
	extern "C" TFMODERN_API bool IsServiceRunning();
	extern "C" TFMODERN_API HRESULT StopService();
	extern "C" TFMODERN_API HRESULT StartService();
	// Installer.
	extern "C" TFMODERN_API HRESULT InstallApp();
	extern "C" TFMODERN_API HRESULT UninstallApp();
}