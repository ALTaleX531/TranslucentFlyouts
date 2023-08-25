#include "pch.h"
#include "Utils.hpp"
#include "TFMain.hpp"
#include "UxThemePatcher.hpp"

using namespace std;
using namespace TranslucentFlyouts;

namespace MainDLL
{
	struct ServiceInfo
	{
		HWND hostWindow{ nullptr };
		HWINEVENTHOOK hook{ nullptr };
	};
	using unique_service_info = unique_ptr < ServiceInfo, decltype([](ServiceInfo* ptr)
	{
		if (ptr)
		{
			UnmapViewOfFile(ptr);
		}
	}) > ;

#ifdef _WIN64
	static const wstring_view fileMappingName {L"Local\\TranslucentFlyouts"sv};
#else
	static const wstring_view fileMappingName { L"Local\\TranslucentFlyouts(x86)"sv };
#endif

	static const UINT WM_TFSTOP{ RegisterWindowMessageW(L"TranslucentFlyouts.Stop") };

	// Installer.
	HRESULT Install();
	HRESULT Uninstall();
	// Global hook.
	HRESULT InstallHook();
	HRESULT UninstallHook();
	// Host.
	HRESULT StopService();
	HRESULT StartService();

	inline bool IsCurrentProcessInBlockList();
	inline bool IsServiceRunning();
	inline unique_service_info GetServiceInfo();
}

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
			break;
		}

		case DLL_PROCESS_DETACH:
		{
			TFMain::Shutdown();

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

	HRESULT hr{S_OK};

	if (!_stricmp(lpCmdLine, "/stop"))
	{
		hr = MainDLL::StopService();

		ShellMessageBoxW(
			hInstance,
			nullptr,
			Utils::MakeHRErrStr(hr).c_str(),
			nullptr,
			FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
		);

		return hr;
	}

	if (!_stricmp(lpCmdLine, "/install"))
	{
		hr = MainDLL::Install();

		ShellMessageBoxW(
			hInstance,
			nullptr,
			Utils::MakeHRErrStr(hr).c_str(),
			nullptr,
			FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
		);

		hr = MainDLL::StartService();

		if (FAILED(hr))
		{
			ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::MakeHRErrStr(hr).c_str(),
				nullptr,
				MB_ICONINFORMATION
			);
		}

		return hr;
	}

	if (!_stricmp(lpCmdLine, "/uninstall"))
	{
		hr = []()
		{
			if (MainDLL::IsServiceRunning())
			{
				RETURN_IF_FAILED(MainDLL::StopService());
			}
			RETURN_IF_FAILED(MainDLL::Uninstall());

			return S_OK;
		}
		();

		ShellMessageBoxW(
			hInstance,
			nullptr,
			Utils::MakeHRErrStr(hr).c_str(),
			nullptr,
			FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION
		);

		return hr;
	}

	if (!_stricmp(lpCmdLine, "/start"))
	{
		hr = MainDLL::StartService();

		if (FAILED(hr))
		{
			ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::MakeHRErrStr(hr).c_str(),
				nullptr,
				MB_ICONERROR
			);
		}

		return hr;
	}

	ShellMessageBoxW(
		hInstance,
		nullptr,
		Utils::MakeHRErrStr(HRESULT_FROM_WIN32(ERROR_BAD_COMMAND)).c_str(),
		nullptr,
		MB_ICONERROR
	);
	return HRESULT_FROM_WIN32(ERROR_BAD_COMMAND);
}

HRESULT MainDLL::StartService()
{
	wil::unique_handle fileMapping{ nullptr };
	unique_service_info serviceInfo{nullptr};

	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING), IsServiceRunning());
	fileMapping.reset(CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(ServiceInfo), fileMappingName.data()));
	RETURN_LAST_ERROR_IF_NULL(fileMapping);
	serviceInfo.reset(reinterpret_cast<ServiceInfo*>(MapViewOfFile(fileMapping.get(), FILE_MAP_ALL_ACCESS, 0, 0, 0)));
	RETURN_LAST_ERROR_IF_NULL(serviceInfo);
	RETURN_IF_FAILED(MainDLL::InstallHook());

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
	serviceInfo->hostWindow = CreateWindowExW(
								  WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
								  L"Static",
								  nullptr,
								  WS_POPUP,
								  0, 0, 0, 0,
								  nullptr, nullptr, HINST_THISCOMPONENT, nullptr
							  );
	RETURN_LAST_ERROR_IF_NULL(serviceInfo->hostWindow);
	RETURN_IF_WIN32_BOOL_FALSE(ChangeWindowMessageFilterEx(serviceInfo->hostWindow, WM_TFSTOP, MSGFLT_ALLOW, nullptr));

	auto callback = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) -> LRESULT
	{
		if (uMsg == WM_ENDSESSION || uMsg == WM_TFSTOP)
		{
			DestroyWindow(hWnd);
		}
		if (uMsg == WM_DESTROY)
		{
			PostQuitMessage(0);
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	};
	RETURN_HR_IF(E_FAIL, !SetWindowSubclass(serviceInfo->hostWindow, callback, 0, 0));
	RETURN_IF_WIN32_BOOL_FALSE(SetProcessShutdownParameters(0, 0));

	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		DispatchMessage(&msg);
	}
#endif // _DEBUG

	serviceInfo->hostWindow = nullptr;

	return MainDLL::UninstallHook();
}

HRESULT MainDLL::InstallHook()
{
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE), !IsServiceRunning());
	auto serviceInfo{ GetServiceInfo() };
	RETURN_LAST_ERROR_IF_NULL(serviceInfo);
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), serviceInfo->hook != nullptr);

	TFMain::Prepare();

	serviceInfo->hook = SetWinEventHook(
							EVENT_MIN, EVENT_MAX,
							HINST_THISCOMPONENT,
							TFMain::HandleWinEvent,
							0, 0,
							WINEVENT_INCONTEXT
						);
	RETURN_LAST_ERROR_IF_NULL(serviceInfo->hook);

	return S_OK;
}

HRESULT MainDLL::UninstallHook()
{
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE), !IsServiceRunning());
	auto serviceInfo{ GetServiceInfo() };
	RETURN_LAST_ERROR_IF_NULL(serviceInfo);
	RETURN_HR_IF_NULL(HRESULT_FROM_WIN32(ERROR_HOOK_NOT_INSTALLED), serviceInfo->hook);
	RETURN_IF_WIN32_BOOL_FALSE(UnhookWinEvent(serviceInfo->hook));
	serviceInfo->hook = nullptr;

	BroadcastSystemMessageW(
		BSF_FORCEIFHUNG | BSF_FLUSHDISK | BSF_POSTMESSAGE,
		nullptr,
		RegisterWindowMessageW(L"ShellFileOpened"),
		0,
		0
	);

	return S_OK;
}

bool MainDLL::IsCurrentProcessInBlockList()
{
	// TranslucentFlyouts won't be loaded into one of these process
	// These processes are quite annoying because TranslucentFlyouts will not be automatically unloaded by them
	// Some of them even have no chance to show flyouts and other UI elements
	static const array blockList
	{
		L"sihost.exe"sv,
		L"WSHost.exe"sv,
		L"spoolsv.exe"sv,
		L"dllhost.exe"sv,
		L"svchost.exe"sv,
		L"taskhostw.exe"sv,
		L"searchhost.exe"sv,
		L"backgroundTaskHost.exe"sv,
		L"RuntimeBroker.exe"sv,
		L"smartscreen.exe"sv,
		L"Widgets.exe"sv,
		L"WidgetService.exe"sv,
		L"GameBar.exe"sv,
		L"GameBarFTServer.exe"sv,
		L"ShellExperienceHost.exe"sv,
		L"StartMenuExperienceHost.exe"sv,
		L"WindowsPackageManagerServer.exe"sv,
		L"msedgewebview2.exe"sv,
		L"Microsoft.SharePoint.exe"sv
	};
	auto is_in_list = [](const auto list)
	{
		for (auto item : list)
		{
			if (GetModuleHandleW(item.data()))
			{
				return true;
			}
		}

		return false;
	};

	return is_in_list(blockList);
}

bool MainDLL::IsServiceRunning()
{
	return wil::unique_handle{ OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, fileMappingName.data()) }.get() != nullptr;
}

MainDLL::unique_service_info MainDLL::GetServiceInfo()
{
	wil::unique_handle fileMapping{ OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, fileMappingName.data()) };
	return unique_service_info{ reinterpret_cast<ServiceInfo*>(MapViewOfFile(fileMapping.get(), FILE_MAP_ALL_ACCESS, 0, 0, 0)) };
}

HRESULT MainDLL::StopService()
{
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE), !IsServiceRunning());
	auto serviceInfo{ GetServiceInfo() };
	RETURN_LAST_ERROR_IF_NULL(serviceInfo);
	RETURN_HR_IF_NULL(HRESULT_FROM_WIN32(ERROR_SERVICE_START_HANG), serviceInfo->hostWindow);
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), !IsWindow(serviceInfo->hostWindow));
	RETURN_LAST_ERROR_IF(!SendNotifyMessageW(serviceInfo->hostWindow, WM_TFSTOP, 0, 0));

	UINT frameCount{0};
	constexpr UINT maxWaitFrameCount{1500};
	while (serviceInfo->hostWindow && frameCount < maxWaitFrameCount)
	{
		DwmFlush();
		if ((++frameCount) == maxWaitFrameCount)
		{
			RETURN_HR(HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT));
		}
	}

	return S_OK;
}

HRESULT MainDLL::Install() try
{
	using namespace wil;
	using namespace TranslucentFlyouts;

	HRESULT hr{S_OK};
	auto CleanUp{Utils::RoInit(&hr)};
	THROW_IF_FAILED(hr);

	{
		com_ptr<ITaskService> taskService{wil::CoCreateInstance<ITaskService>(CLSID_TaskScheduler)};
		THROW_IF_FAILED(taskService->Connect(_variant_t{}, _variant_t{}, _variant_t{}, _variant_t{}));

		com_ptr<ITaskFolder> rootFolder{nullptr};
		THROW_IF_FAILED(taskService->GetFolder(_bstr_t("\\"), &rootFolder));

		com_ptr<ITaskDefinition> taskDefinition{nullptr};
		THROW_IF_FAILED(taskService->NewTask(0, &taskDefinition));

		com_ptr<IRegistrationInfo> regInfo{nullptr};
		THROW_IF_FAILED(taskDefinition->get_RegistrationInfo(&regInfo));
		THROW_IF_FAILED(regInfo->put_Author(const_cast<BSTR>(L"ALTaleX")));
		THROW_IF_FAILED(regInfo->put_Description(const_cast<BSTR>(L"This task provide translucent support for specific flyouts.")));

		{
			com_ptr<IPrincipal> principal{nullptr};
			THROW_IF_FAILED(taskDefinition->get_Principal(&principal));

			THROW_IF_FAILED(principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN));
			THROW_IF_FAILED(principal->put_RunLevel(TASK_RUNLEVEL_HIGHEST));
		}

		{
			com_ptr<ITaskSettings> setting{nullptr};
			THROW_IF_FAILED(taskDefinition->get_Settings(&setting));

			THROW_IF_FAILED(setting->put_StopIfGoingOnBatteries(VARIANT_FALSE));
			THROW_IF_FAILED(setting->put_DisallowStartIfOnBatteries(VARIANT_FALSE));
			THROW_IF_FAILED(setting->put_AllowDemandStart(VARIANT_TRUE));
			THROW_IF_FAILED(setting->put_StartWhenAvailable(VARIANT_FALSE));
			THROW_IF_FAILED(setting->put_MultipleInstances(TASK_INSTANCES_PARALLEL));
		}

		{
			com_ptr<IExecAction> execAction{nullptr};
			{
				com_ptr<IAction> action{nullptr};
				{
					com_ptr<IActionCollection> actionColl{nullptr};
					THROW_IF_FAILED(taskDefinition->get_Actions(&actionColl));
					THROW_IF_FAILED(actionColl->Create(TASK_ACTION_EXEC, &action));
				}
				action.query_to(&execAction);
			}

			WCHAR modulePath[MAX_PATH + 1] {};
			RETURN_LAST_ERROR_IF(!GetModuleFileName(HINST_THISCOMPONENT, modulePath, MAX_PATH));

			THROW_IF_FAILED(
				execAction->put_Path(
					const_cast<BSTR>(L"Rundll32")
				)
			);

			THROW_IF_FAILED(
				execAction->put_Arguments(
					const_cast<BSTR>(
						std::format(L"\"{}\",Main /start", modulePath).c_str()
					)
				)
			);
		}

		com_ptr<ITriggerCollection> triggerColl{nullptr};
		THROW_IF_FAILED(taskDefinition->get_Triggers(&triggerColl));

		com_ptr<ITrigger> trigger{nullptr};
		THROW_IF_FAILED(triggerColl->Create(TASK_TRIGGER_LOGON, &trigger));

		com_ptr<IRegisteredTask> registeredTask{nullptr};
		BSTR name
		{
#ifdef _WIN64
			const_cast<BSTR>(L"TranslucentFlyouts Autorun Task")
#else
			const_cast<BSTR>(L"TranslucentFlyouts Autorun Task (x86)")
#endif
		};
		THROW_IF_FAILED(
			rootFolder->RegisterTaskDefinition(
				name,
				taskDefinition.get(),
				TASK_CREATE_OR_UPDATE,
				_variant_t{},
				_variant_t{},
				TASK_LOGON_INTERACTIVE_TOKEN,
				_variant_t{},
				& registeredTask
			)
		);
	}

	return S_OK;
}
CATCH_LOG_RETURN_HR(wil::ResultFromCaughtException())

HRESULT MainDLL::Uninstall() try
{
	using namespace wil;
	using namespace TranslucentFlyouts;

	HRESULT hr{S_OK};
	auto CleanUp{Utils::RoInit(&hr)};
	THROW_IF_FAILED(hr);

	{
		com_ptr<ITaskService> taskService{wil::CoCreateInstance<ITaskService>(CLSID_TaskScheduler)};
		THROW_IF_FAILED(taskService->Connect(_variant_t{}, _variant_t{}, _variant_t{}, _variant_t{}));

		com_ptr<ITaskFolder> rootFolder{nullptr};
		THROW_IF_FAILED(taskService->GetFolder(_bstr_t("\\"), &rootFolder));

		BSTR name
		{
#ifdef _WIN64
			const_cast<BSTR>(L"TranslucentFlyouts Autorun Task")
#else
			const_cast<BSTR>(L"TranslucentFlyouts Autorun Task (x86)")
#endif
		};
		THROW_IF_FAILED(
			rootFolder->DeleteTask(
				name, 0
			)
		);
	}

	return S_OK;
}
CATCH_LOG_RETURN_HR(wil::ResultFromCaughtException())