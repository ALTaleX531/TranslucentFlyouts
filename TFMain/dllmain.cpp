#include "pch.h"
#include "TFMain.hpp"

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  dwReason,
	LPVOID lpReserved
)
{
	using namespace TranslucentFlyouts;
	BOOL bResult{TRUE};

	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hModule);

			if (MainDLL::IsCurrentProcessInBlockList())
			{
				bResult = FALSE;
			}
			else
			{
				if (
					MainDLL::IsHookGlobalInstalled() &&
					//ThemeHelper::IsThemeAvailable() &&
					//!ThemeHelper::IsHighContrast() &&
					!GetSystemMetrics(SM_CLEANBOOT)
				)
				{
					MainDLL::GetInstance().Startup();
				}
			}
			break;
		}

		case DLL_PROCESS_DETACH:
		{
			MainDLL::GetInstance().Shutdown();

			break;
		}
		default:
			break;
	}

	return bResult;
}

int WINAPI Main(
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

	if (!_stricmp(lpCmdLine, "/install"))
	{
		WCHAR modulePath[MAX_PATH + 1] {};
		RETURN_LAST_ERROR_IF(!GetModuleFileName(HINST_THISCOMPONENT, modulePath, MAX_PATH));

		auto parameters
		{
#ifdef _WIN64
			std::format(
				L"/create /sc ONLOGON /tn \"TranslucentFlyouts Autorun Task\" /rl HIGHEST /tr \"Rundll32 \\\"{}\\\",Main\"",
				modulePath
			)
#else
			std::format(
				L"/create /sc ONLOGON /tn \"TranslucentFlyouts Autorun Task (x86)\" /rl HIGHEST /tr \"Rundll32 \\\"{}\\\",Main\"",
				modulePath
			)
#endif
		};
		SHELLEXECUTEINFOW sei
		{
			sizeof(SHELLEXECUTEINFO), SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC,
			nullptr,
			L"runas",
			L"schtasks",
			parameters.c_str(),
			nullptr,
			SW_HIDE
		};

		if (!ShellExecuteExW(&sei))
		{
			RETURN_LAST_ERROR();
		}

		RestartDialog(nullptr, nullptr, EWX_LOGOFF);
		return 0;
	}

	if (!_stricmp(lpCmdLine, "/uninstall"))
	{
		SHELLEXECUTEINFOW sei
		{
			sizeof(SHELLEXECUTEINFO), SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC,
			nullptr,
			L"runas",
			L"schtasks",
#ifdef _WIN64
			L"/delete /f /tn \"TranslucentFlyouts Autorun Task\"",
#else
			L"/delete /f /tn \"TranslucentFlyouts Autorun Task (x86)\"",
#endif
			nullptr,
			SW_HIDE
		};

		if (!ShellExecuteExW(&sei))
		{
			RETURN_LAST_ERROR();
		}

		RestartDialog(nullptr, nullptr, EWX_LOGOFF);
		return 0;
	}

	HRESULT hr{MainDLL::InstallHook()};
	if (FAILED(hr))
	{
		return hr;
	}

#ifdef _DEBUG
	Utils::StartupConsole();
	{
		printf("Input 'Q' or 'q' to exit...\n");

		char input{};
		do
		{
			input = getchar();
		}
		while (input != 'q' && input != 'Q');

		Utils::ShutdownConsole();
	}
#else
	SetProcessShutdownParameters(0, 0);
	HWND window
	{
		CreateWindowExW(
			WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
			L"#32770",
			nullptr,
			WS_POPUP,
			0, 0, 0, 0,
			nullptr, nullptr, HINST_THISCOMPONENT, nullptr
		)
	};
	RETURN_LAST_ERROR_IF_NULL(window);
	auto callback = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) -> LRESULT
	{
		if (uMsg == WM_ENDSESSION)
		{
			DestroyWindow(hWnd);
		}
		if (uMsg == WM_DESTROY)
		{
			PostQuitMessage(0);
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	};
	RETURN_IF_WIN32_BOOL_FALSE(SetWindowSubclass(window, callback, 0, 0));
	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		DispatchMessage(&msg);
	}
#endif // _DEBUG

	return MainDLL::UninstallHook();
}