#include "pch.h"
#include "resource.h"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "Framework.hpp"
#include "SystemHelper.hpp"
#include "Application.hpp"

using namespace TranslucentFlyouts;

LPTOP_LEVEL_EXCEPTION_FILTER g_old{ nullptr };
LONG NTAPI TopLevelExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
	LONG result{ g_old ? g_old(exceptionInfo) : EXCEPTION_CONTINUE_SEARCH };

	HRESULT hr = [exceptionInfo]()
	{
		CreateDirectoryW(Utils::make_current_folder_file_str(L"dumps").c_str(), nullptr);

		std::time_t tt{ std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
		tm _tm{};
		WCHAR time[MAX_PATH + 1];
		localtime_s(&_tm, &tt);
		std::wcsftime(time, MAX_PATH, L"%Y-%m-%d-%H-%M-%S", &_tm);

		wil::unique_hfile fileHandle
		{
			CreateFile2(
				Utils::make_current_folder_file_str(
					std::format(
						L"dumps\\{}-minidump-{}.dmp",
						Utils::get_process_name(),
						time
					)
				).c_str(),
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ,
				CREATE_ALWAYS,
				nullptr
			)
		};
		RETURN_LAST_ERROR_IF(!fileHandle.is_valid());

		MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInfo{ GetCurrentThreadId(), exceptionInfo, FALSE };
		RETURN_IF_WIN32_BOOL_FALSE(
			MiniDumpWriteDump(
				GetCurrentProcess(),
				GetCurrentProcessId(),
				fileHandle.get(),
				static_cast<MINIDUMP_TYPE>(
					MINIDUMP_TYPE::MiniDumpWithThreadInfo |
					MINIDUMP_TYPE::MiniDumpWithUnloadedModules |
					MINIDUMP_TYPE::MiniDumpWithHandleData
					),
				&minidumpExceptionInfo,
				nullptr,
				nullptr
			)
		);

		return S_OK;
	} ();
		
	if (SUCCEEDED(hr))
	{
		if (
			MessageBoxW(
				nullptr,
				Utils::GetResWString<IDS_STRING110>().c_str(),
				nullptr,
				MB_ICONERROR | MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION | MB_SETFOREGROUND | MB_YESNO
			) == IDYES
		)
		{
			std::thread{ []
			{
				Application::StopService();
			} }.detach();
		}
	}

	return result;
}

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
				Api::IsCurrentProcessInBlockList() ||
				GetSystemMetrics(SM_CLEANBOOT) ||
				RegHelper::Get<DWORD>({ L"BlockList" }, Utils::get_process_name(), 0)
			)
			{
				return FALSE;
			}
			else if (Api::IsServiceRunning(Application::g_serviceName))
			{
				if (RegHelper::Get<DWORD>({}, L"EnableMiniDump", 1) || !_wcsicmp(Utils::get_process_name().c_str(), L"explorer.exe"))
				{
					g_old = SetUnhandledExceptionFilter(TopLevelExceptionFilter);
				}
				Framework::Startup();
			}
			break;
		}

		case DLL_PROCESS_DETACH:
		{
			if (g_old)
			{
				SetUnhandledExceptionFilter(g_old);
			}
			Framework::Shutdown();

			break;
		}
		default:
			break;
	}

	wil::DLLMain(hModule, dwReason, lpReserved);
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
	RETURN_IF_WIN32_BOOL_FALSE(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
	RETURN_IF_FAILED(SetCurrentProcessExplicitAppUserModelID(L"TranslucentFlyouts.Win32.Service"));

	HRESULT hr{S_OK};

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