#include "pch.h"
#include "resource.h"
#include "Utils.hpp"
#include "TFMain.hpp"
#include "UxThemePatcher.hpp"

using namespace std;
using namespace TranslucentFlyouts;

namespace MainDLL
{
	// TranslucentFlyouts won't be loaded into one of these process
	// These processes are quite annoying because TranslucentFlyouts will not be automatically unloaded by them
	// Some of them even have no chance to show flyouts and other UI elements
	const array g_blockList
	{
		L"sihost.exe"sv,
		L"WSHost.exe"sv,
		L"spoolsv.exe"sv,
		L"dllhost.exe"sv,
		L"svchost.exe"sv,
		L"searchhost.exe"sv,
		L"taskhostw.exe"sv,
		L"searchhost.exe"sv,
		L"RuntimeBroker.exe"sv,
		L"smartscreen.exe"sv,
		L"Widgets.exe"sv,
		L"WidgetService.exe"sv,
		L"GameBar.exe"sv,
		L"GameBarFTServer.exe"sv,
		L"ShellExperienceHost.exe"sv,
		L"StartMenuExperienceHost.exe"sv,
		L"msedgewebview2.exe"sv,
		L"Microsoft.SharePoint.exe"sv
	};

#pragma data_seg(".shared")
	HWND g_serviceWindow{ nullptr };
	HWINEVENTHOOK g_hHook{ nullptr };
#pragma data_seg()
#pragma comment(linker,"/SECTION:.shared,RWS")
	static const UINT WM_TFSTOP{ RegisterWindowMessageW(L"TranslucentFlyouts.Stop") };

	// Installer.
	HRESULT WINAPI Install();
	HRESULT WINAPI Uninstall();
	// Global hook.
	HRESULT InstallHook();
	HRESULT UninstallHook();

	bool IsCurrentProcessInBlockList();
	inline bool IsHookGlobalInstalled();
	// Host.
	HRESULT WINAPI StopService();
	HRESULT WINAPI StartService();
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
			if (MainDLL::g_serviceWindow)
			{
				RETURN_IF_FAILED(MainDLL::StopService());
			}
			RETURN_IF_FAILED(MainDLL::Uninstall());

			return S_OK;
		}();

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

HRESULT WINAPI MainDLL::StartService()
{
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
	g_serviceWindow = CreateWindowExW(
						  WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
						  L"Static",
						  nullptr,
						  WS_POPUP,
						  0, 0, 0, 0,
						  nullptr, nullptr, HINST_THISCOMPONENT, nullptr
					  );
	RETURN_LAST_ERROR_IF_NULL(g_serviceWindow);
	RETURN_IF_WIN32_BOOL_FALSE(ChangeWindowMessageFilterEx(g_serviceWindow, WM_TFSTOP, MSGFLT_ALLOW, nullptr));

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
	RETURN_IF_WIN32_BOOL_FALSE(SetWindowSubclass(g_serviceWindow, callback, 0, 0));
	RETURN_IF_WIN32_BOOL_FALSE(SetProcessShutdownParameters(0, 0));

	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		DispatchMessage(&msg);
	}
#endif // _DEBUG

	g_serviceWindow = nullptr;

	return MainDLL::UninstallHook();
}

HRESULT MainDLL::InstallHook()
{
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), IsHookGlobalInstalled());

	UxThemePatcher::PrepareUxTheme();
	{
		if (GetConsoleWindow())
		{
			Utils::OutputModuleString(IDS_STRING104);
			system("pause>nul");

			Utils::ShutdownConsole();
		}
	}
	g_hHook = SetWinEventHook(
		EVENT_MIN, EVENT_MAX,
		HINST_THISCOMPONENT,
		TFMain::HandleWinEvent,
		0, 0,
		WINEVENT_INCONTEXT
	);
	RETURN_LAST_ERROR_IF_NULL(g_hHook);

	return S_OK;
}

HRESULT MainDLL::UninstallHook()
{
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_HOOK_NOT_INSTALLED), !IsHookGlobalInstalled());
	RETURN_IF_WIN32_BOOL_FALSE(UnhookWinEvent(g_hHook));
	g_hHook = nullptr;

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
	for (auto processName : g_blockList)
	{
		if (GetModuleHandleW(processName.data()))
		{
			return true;
		}
	}

	return false;
}

bool MainDLL::IsHookGlobalInstalled()
{
	return g_hHook != nullptr;
}

HRESULT WINAPI MainDLL::StopService()
{
	RETURN_HR_IF_EXPECTED(HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE), !g_serviceWindow);
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), !IsWindow(g_serviceWindow));
	RETURN_LAST_ERROR_IF(!SendNotifyMessageW(g_serviceWindow, WM_TFSTOP, 0, 0));

	UINT frameCount{0};
	constexpr UINT maxWaitFrameCount{1500};
	while (g_serviceWindow && frameCount < maxWaitFrameCount)
	{
		DwmFlush();
		if ((++frameCount) == maxWaitFrameCount)
		{
			RETURN_HR(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
		}
	}

	return S_OK;
}

HRESULT WINAPI MainDLL::Install() try
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
		THROW_IF_FAILED(regInfo->put_Description(const_cast<BSTR>(L"This task ensures most of the flyout translucent all the time.")));

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

#ifdef _WIN64
			THROW_IF_FAILED(
				execAction->put_Path(
					const_cast<BSTR>(L"C:\\Windows\\System32\\Rundll32.exe")
				)
			);
#else
			THROW_IF_FAILED(
				execAction->put_Path(
					const_cast<BSTR>(L"C:\\Windows\\SysWOW64\\Rundll32.exe")
				)
			);
#endif
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

HRESULT WINAPI MainDLL::Uninstall() try
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