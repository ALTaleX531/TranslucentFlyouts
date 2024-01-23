#include "pch.h"
#include "Api.hpp"
#include "Utils.hpp"
#include "Framework.hpp"
#include "Application.hpp"

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  dwReason,
	LPVOID lpReserved
)
{
	using namespace TranslucentFlyouts;
	BOOL bResult{ TRUE };

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);

		if (
			GetSystemMetrics(SM_CLEANBOOT)
		)
		{
			return FALSE;
		}
		else if (
			!GetModuleHandleW(L"Rundll32.exe") &&
			!GetModuleHandleW(L"TranslucentFlyoutsConfig.exe")
		)
		{
			Framework::Startup();
		}
		break;
	}

	case DLL_PROCESS_DETACH:
	{
		Framework::Shutdown();
		break;
	}
	default:
		break;
	}

	wil::DLLMain(hModule, dwReason, lpReserved);
	return bResult;
}

extern "C" TFMODERN_API int WINAPI Main(
	HWND hWnd,
	HINSTANCE hInstance,
	LPCSTR    lpCmdLine,
	int       nCmdShow
)
{
	using namespace TranslucentFlyouts;

	// Until now, We only support Chinese and English...
	if (GetUserDefaultUILanguage() != MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
	{
		SetThreadUILanguage(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	}
	RETURN_IF_WIN32_BOOL_FALSE(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));

	HRESULT hr{ S_OK };

	if (!_stricmp(lpCmdLine, "/stop"))
	{
		hr = Application::StopService();

		ShellMessageBoxW(
			hInstance,
			nullptr,
			Utils::to_error_wstring(hr).c_str(),
			nullptr,
			FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
		);

		return hr;
	}

	if (GetSystemMetrics(SM_CLEANBOOT))
	{
		ShellMessageBoxW(
			hInstance,
			nullptr,
			Utils::to_error_wstring(HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED)).c_str(),
			nullptr,
			MB_ICONERROR
		);
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

	if (!_stricmp(lpCmdLine, "/install"))
	{
		hr = Application::InstallApp();

		ShellMessageBoxW(
			hInstance,
			nullptr,
			Utils::to_error_wstring(hr).c_str(),
			nullptr,
			FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
		);

		if (FAILED(hr))
		{
			return hr;
		}

		hr = Application::StartService();

		if (FAILED(hr))
		{
			ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::to_error_wstring(hr).c_str(),
				nullptr,
				MB_ICONERROR
			);
		}

		return hr;
	}

	if (!_stricmp(lpCmdLine, "/uninstall"))
	{
		hr = []()
		{
			if (Api::IsServiceRunning(Application::g_serviceName))
			{
				RETURN_IF_FAILED(Application::StopService());
			}
			RETURN_IF_FAILED(Application::UninstallApp());

			return S_OK;
		}
		();

		ShellMessageBoxW(
			hInstance,
			nullptr,
			Utils::to_error_wstring(hr).c_str(),
			nullptr,
			FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
		);

		return hr;
	}

	if (!_stricmp(lpCmdLine, "/start"))
	{
		hr = Application::StartService();

		if (FAILED(hr))
		{
			ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::to_error_wstring(hr).c_str(),
				nullptr,
				MB_ICONERROR
			);
		}

		return hr;
	}

	ShellMessageBoxW(
		hInstance,
		nullptr,
		Utils::to_error_wstring(HRESULT_FROM_WIN32(ERROR_BAD_DRIVER_LEVEL)).c_str(),
		nullptr,
		MB_ICONERROR
	);
	return HRESULT_FROM_WIN32(ERROR_BAD_DRIVER_LEVEL);
}