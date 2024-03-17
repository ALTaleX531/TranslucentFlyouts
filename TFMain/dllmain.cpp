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
					(RegHelper::Get<DWORD>({}, L"MiniDumpWithFullMemory", 0) ? MINIDUMP_TYPE::MiniDumpWithFullMemory : MINIDUMP_TYPE::MiniDumpWithoutOptionalData) |
					MINIDUMP_TYPE::MiniDumpWithUnloadedModules
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
				if (
					(
						RegHelper::Get<DWORD>({}, L"EnableMiniDump", 1) &&
						!RegHelper::Get<DWORD>({ L"DisabledList" }, Utils::get_process_name(), 0)
					) ||
					!_wcsicmp(Utils::get_process_name().c_str(), L"explorer.exe")
				)
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

struct ExecutionParameters
{
	enum class CommandType
	{
		Unknown,
		Stop,
		Start,
		Install,
		Uninstall,
		Kill,
		ClearCache,
	} type{ CommandType::Unknown };
	bool silent{ false };
};

ExecutionParameters AnalyseCommandLine(LPCWSTR lpCmdLine)
{
	int args{ 0 };
	auto argv{ CommandLineToArgvW(lpCmdLine, &args) };
	ExecutionParameters params{};

	for (int i = 0; i < args; i++)
	{
		if (!_wcsicmp(argv[i], L"/stop") || !_wcsicmp(argv[i], L"/s") || !_wcsicmp(argv[i], L"-stop") || !_wcsicmp(argv[i], L"-s"))
		{
			params.type = ExecutionParameters::CommandType::Stop;
		}
		if (!_wcsicmp(argv[i], L"/start") || !_wcsicmp(argv[i], L"/l") || !_wcsicmp(argv[i], L"-start") || !_wcsicmp(argv[i], L"-l"))
		{
			params.type = ExecutionParameters::CommandType::Start;
		}
		if (!_wcsicmp(argv[i], L"/install") || !_wcsicmp(argv[i], L"/i") || !_wcsicmp(argv[i], L"-install") || !_wcsicmp(argv[i], L"-i"))
		{
			params.type = ExecutionParameters::CommandType::Install;
		}
		if (!_wcsicmp(argv[i], L"/uninstall") || !_wcsicmp(argv[i], L"/u") || !_wcsicmp(argv[i], L"-uninstall") || !_wcsicmp(argv[i], L"-u"))
		{
			params.type = ExecutionParameters::CommandType::Uninstall;
		}
		if (!_wcsicmp(argv[i], L"/kill") || !_wcsicmp(argv[i], L"/k") || !_wcsicmp(argv[i], L"-kill") || !_wcsicmp(argv[i], L"-k"))
		{
			params.type = ExecutionParameters::CommandType::Kill;
		}
		if (!_wcsicmp(argv[i], L"/clearcache") || !_wcsicmp(argv[i], L"/c") || !_wcsicmp(argv[i], L"-clearcache") || !_wcsicmp(argv[i], L"-c"))
		{
			params.type = ExecutionParameters::CommandType::ClearCache;
		}
		if (!_wcsicmp(argv[i], L"/silent") || !_wcsicmp(argv[i], L"/s") || !_wcsicmp(argv[i], L"-silent") || !_wcsicmp(argv[i], L"-s"))
		{
			params.silent = true;
		}
	}

	return params;
}

int WINAPI Main(
	HWND hWnd,
	HINSTANCE hInstance,
	LPCSTR    lpCmdLine,
	int       /*nCmdShow*/
)
{
	using namespace TranslucentFlyouts;

	// Until now, We only support Chinese and English...
	if (PRIMARYLANGID(GetUserDefaultUILanguage()) != LANG_CHINESE)
	{
		SetThreadUILanguage(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	}
	RETURN_IF_WIN32_BOOL_FALSE(SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2));
	RETURN_IF_FAILED(SetCurrentProcessExplicitAppUserModelID(L"TranslucentFlyouts.Win32.Service"));
	Api::InteractiveIO::SetConsoleInitializationCallback([]()
	{
		HWND hwnd{ GetConsoleWindow() };
		[hwnd]()
		{
			PROPVARIANT propVariant{};
			InitPropVariantFromString(L"TranslucentFlyouts.Win32.Service", &propVariant);
			wil::com_ptr<IPropertyStore> propertyStore{ nullptr };
			RETURN_IF_FAILED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(propertyStore.put())));
			RETURN_IF_FAILED(propertyStore->SetValue(PKEY_AppUserModel_ID, propVariant));
			RETURN_IF_FAILED(propertyStore->Commit());
			return S_OK;
		}();
		SetConsoleTitleW(L"TranslucentFlyouts.Win32 Service Console");
		wil::unique_hicon icon{ LoadIconW(GetModuleHandleW(L"shell32.dll"), MAKEINTRESOURCEW(15)) };
		SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon.get()));
		SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon.get()));
		SetClassLongPtrW(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(icon.get()));
	});

	// Convert the ansi string back to unicode string
	HRESULT hr{ S_OK };
	auto length{ MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, nullptr, 0) };
	THROW_LAST_ERROR_IF(length == 0);
	wil::unique_cotaskmem_string convertedCommandLine{ reinterpret_cast<PWSTR>(CoTaskMemAlloc(sizeof(WCHAR) * length)) };
	THROW_LAST_ERROR_IF_NULL(convertedCommandLine);
	THROW_LAST_ERROR_IF(MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, convertedCommandLine.get(), length) == 0);

	auto params{ AnalyseCommandLine(convertedCommandLine.get()) };

	if (params.type == ExecutionParameters::CommandType::ClearCache)
	{
		Application::ClearCache();

		return S_OK;
	}
	if (params.type == ExecutionParameters::CommandType::Kill)
	{
		hr = Application::KillService();

		if (!params.silent)
		{
			ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::to_error_wstring(hr).c_str(),
				nullptr,
				FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
			);
		}

		return hr;
	}
	if (params.type == ExecutionParameters::CommandType::Stop)
	{
		hr = Application::StopService();

		if (!params.silent)
		{
			ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::to_error_wstring(hr).c_str(),
				nullptr,
				FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
			);
		}

		return hr;
	}
	if (params.type == ExecutionParameters::CommandType::Uninstall)
	{
		hr = []()
		{
			if (Api::IsServiceRunning(Application::g_serviceName))
			{
				RETURN_IF_FAILED(Application::StopService());
			}
			RETURN_IF_FAILED(Application::UninstallApp());

			return S_OK;
		} ();

		if (!params.silent)
		{
			ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::to_error_wstring(hr).c_str(),
				nullptr,
				FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
			);
		}

		return hr;
	}
	// Running in safe mode?
	if (GetSystemMetrics(SM_CLEANBOOT))
	{
		if (!params.silent)
		{
			ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::to_error_wstring(HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED)).c_str(),
				nullptr,
				MB_ICONERROR
			);
		}
		return HRESULT_FROM_WIN32(ERROR_SERVICE_DISABLED);
	}

	if (params.type == ExecutionParameters::CommandType::Install)
	{
		WCHAR userName[MAX_PATH + 1]{};
		DWORD bufferSize{ MAX_PATH };
		GetUserNameW(userName, &bufferSize);

		if (!params.silent)
		{
			if (
				ShellMessageBoxW(
					hInstance,
					nullptr,
					Utils::GetResWString<IDS_STRING111>().c_str(),
					(Utils::GetResWString<IDS_STRING112>() + userName).c_str(),
					MB_ICONINFORMATION | MB_YESNO
				) == IDNO
				)
			{
				return E_ABORT;
			}
		}

		hr = Application::InstallApp();

		if (!params.silent)
		{
			ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::to_error_wstring(hr).c_str(),
				nullptr,
				FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
			);
		}

		if (FAILED(hr))
		{
			return hr;
		}

		hr = Application::StartService(hWnd);

		if (FAILED(hr))
		{
			if (!params.silent)
			{
				ShellMessageBoxW(
					hInstance,
					nullptr,
					Utils::to_error_wstring(hr).c_str(),
					nullptr,
					MB_ICONERROR
				);
			}
		}

		return hr;
	}
	if (params.type == ExecutionParameters::CommandType::Start)
	{
		hr = Application::StartService();

		if (FAILED(hr))
		{
			if (!params.silent)
			{
				ShellMessageBoxW(
					hInstance,
					nullptr,
					Utils::to_error_wstring(hr).c_str(),
					nullptr,
					MB_ICONERROR
				);
			}
		}

		return hr;
	}

	if (!params.silent)
	{
		ShellMessageBoxW(
			hInstance,
			nullptr,
			Utils::to_error_wstring(HRESULT_FROM_WIN32(ERROR_BAD_DRIVER_LEVEL)).c_str(),
			nullptr,
			MB_ICONERROR
		);
	}
	return HRESULT_FROM_WIN32(ERROR_BAD_DRIVER_LEVEL);
}