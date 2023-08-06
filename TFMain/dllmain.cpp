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

HRESULT WINAPI Install() try
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

			THROW_IF_FAILED(
				execAction->put_Path(
					const_cast<BSTR>(L"Rundll32")
				)
			);
			THROW_IF_FAILED(
				execAction->put_Arguments(
					const_cast<BSTR>(
						std::format(L"\"{}\",Main", modulePath).c_str()
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

HRESULT WINAPI Uninstall() try
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

	SHELLEXECUTEINFOA sei
	{
		sizeof(SHELLEXECUTEINFO), SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC,
		nullptr,
		"runas",
		"rundll32",
		lpCmdLine,
		nullptr,
		SW_HIDE
	};

	HRESULT hr{S_OK};
	if (!_stricmp(lpCmdLine, "/install"))
	{
		hr = Install();
		if (SUCCEEDED(hr))
		{
			RestartDialog(nullptr, nullptr, EWX_LOGOFF);
		}
		else
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

	if (!_stricmp(lpCmdLine, "/uninstall"))
	{
		hr = Uninstall();
		if (SUCCEEDED(hr))
		{
			RestartDialog(nullptr, nullptr, EWX_LOGOFF);
		}
		else
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

	hr = MainDLL::InstallHook();
	if (FAILED(hr))
	{
		ShellMessageBoxW(
				hInstance,
				nullptr,
				Utils::MakeHRErrStr(hr).c_str(),
				nullptr,
				MB_ICONERROR
		);
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