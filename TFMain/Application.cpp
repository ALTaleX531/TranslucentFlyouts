﻿#include "pch.h"
#include "resource.h"
#include "Utils.hpp"
#include "Api.hpp"
#include "Framework.hpp"
#include "Application.hpp"

using namespace TranslucentFlyouts;

namespace TranslucentFlyouts::Application
{
	UINT GetStopMsg()
	{
		const UINT TFM_STOP{ RegisterWindowMessageW(L"TranslucentFlyouts.Win32.Stop") };
		return TFM_STOP;
	}
}

HRESULT Application::InstallHook()
{
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE), !Api::IsServiceRunning(g_serviceName));
	auto serviceInfo{ Api::GetServiceInfo(g_serviceName, false) };
	RETURN_LAST_ERROR_IF_NULL(serviceInfo);
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), serviceInfo->hook != nullptr);

	serviceInfo->hook = SetWinEventHook(
		EVENT_OBJECT_CREATE, EVENT_OBJECT_HIDE,
		wil::GetModuleInstanceHandle(),
		Framework::HandleWinEvent,
		0, 0,
		WINEVENT_INCONTEXT
	);
	RETURN_LAST_ERROR_IF_NULL(serviceInfo->hook);
	SendNotifyMessageW(HWND_BROADCAST, WM_NULL, 0, 0);

	return S_OK;
}

HRESULT Application::UninstallHook()
{
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE), !Api::IsServiceRunning(g_serviceName));
	auto serviceInfo{ Api::GetServiceInfo(g_serviceName, false) };
	RETURN_LAST_ERROR_IF_NULL(serviceInfo);
	RETURN_HR_IF_NULL(HRESULT_FROM_WIN32(ERROR_HOOK_NOT_INSTALLED), serviceInfo->hook);
	RETURN_IF_WIN32_BOOL_FALSE(UnhookWinEvent(serviceInfo->hook));
	serviceInfo->hook = nullptr;
	SendNotifyMessageW(HWND_BROADCAST, WM_NULL, 0, 0);

	return S_OK;
}

HRESULT Application::StartService()
{
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING), Api::IsServiceRunning(g_serviceName));
	auto [serviceHandle, serviceInfo]
	{
		Api::CreateService(g_serviceName)
	};
	RETURN_LAST_ERROR_IF_NULL(serviceInfo);

	Framework::Prepare();
	Api::InteractiveIO::OutputToConsole(
		Api::InteractiveIO::StringType::Notification,
		Api::InteractiveIO::WaitType::WaitAnyKey,
		IDS_STRING104,
		L"\n",
		L"\n",
		true
	);
	Api::InteractiveIO::Shutdown();
	RETURN_IF_FAILED(Application::InstallHook());

	serviceInfo->hostWindow = CreateWindowExW(
		WS_EX_NOREDIRECTIONBITMAP | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
		L"Static",
		nullptr,
		WS_POPUP,
		0, 0, 0, 0,
		nullptr, nullptr, wil::GetModuleInstanceHandle(), nullptr
	);
	RETURN_LAST_ERROR_IF_NULL(serviceInfo->hostWindow);
	RETURN_IF_WIN32_BOOL_FALSE(ChangeWindowMessageFilterEx(serviceInfo->hostWindow, GetStopMsg(), MSGFLT_ALLOW, nullptr));

	auto callback = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) -> LRESULT
	{
		if (uMsg == WM_ENDSESSION || uMsg == GetStopMsg())
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

	serviceInfo->hostWindow = nullptr;

	Application::UninstallHook();
	return S_OK;
}

bool Application::IsServiceRunning()
{
	return Api::IsServiceRunning(g_serviceName);
}

HRESULT Application::StopService()
{
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE), !Api::IsServiceRunning(g_serviceName));
	auto serviceInfo{ Api::GetServiceInfo(g_serviceName, false) };
	RETURN_LAST_ERROR_IF_NULL(serviceInfo);
	RETURN_HR_IF_NULL(HRESULT_FROM_WIN32(ERROR_SERVICE_START_HANG), serviceInfo->hostWindow);
	RETURN_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE), !IsWindow(serviceInfo->hostWindow));
	RETURN_LAST_ERROR_IF(!SendNotifyMessageW(serviceInfo->hostWindow, GetStopMsg(), 0, 0));

	UINT frameCount{ 0 };
	constexpr UINT maxWaitFrameCount{ 1500 };
	while (serviceInfo->hostWindow && frameCount < maxWaitFrameCount)
	{
		Sleep(20);
		if ((++frameCount) == maxWaitFrameCount)
		{
			RETURN_HR(HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT));
		}
	}

	return S_OK;
}

HRESULT Application::InstallApp() try
{
	using namespace wil;
	using namespace TranslucentFlyouts;

	HRESULT hr{ S_OK };
	auto CleanUp{ Utils::RoInit(&hr) };
	THROW_IF_FAILED(hr);

	{
		com_ptr<ITaskService> taskService{ wil::CoCreateInstance<ITaskService>(CLSID_TaskScheduler) };
		THROW_IF_FAILED(taskService->Connect(_variant_t{}, _variant_t{}, _variant_t{}, _variant_t{}));

		com_ptr<ITaskFolder> rootFolder{ nullptr };
		THROW_IF_FAILED(taskService->GetFolder(_bstr_t("\\"), &rootFolder));

		com_ptr<ITaskDefinition> taskDefinition{ nullptr };
		THROW_IF_FAILED(taskService->NewTask(0, &taskDefinition));

		com_ptr<IRegistrationInfo> regInfo{ nullptr };
		THROW_IF_FAILED(taskDefinition->get_RegistrationInfo(&regInfo));
		THROW_IF_FAILED(regInfo->put_Author(const_cast<BSTR>(L"ALTaleX")));
		THROW_IF_FAILED(regInfo->put_Description(const_cast<BSTR>(L"This task provide translucent support for win32 flyouts.")));

		{
			com_ptr<IPrincipal> principal{ nullptr };
			THROW_IF_FAILED(taskDefinition->get_Principal(&principal));

			THROW_IF_FAILED(principal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN));
			THROW_IF_FAILED(principal->put_RunLevel(TASK_RUNLEVEL_HIGHEST));
		}

		{
			com_ptr<ITaskSettings> setting{ nullptr };
			THROW_IF_FAILED(taskDefinition->get_Settings(&setting));

			THROW_IF_FAILED(setting->put_StopIfGoingOnBatteries(VARIANT_FALSE));
			THROW_IF_FAILED(setting->put_DisallowStartIfOnBatteries(VARIANT_FALSE));
			THROW_IF_FAILED(setting->put_AllowDemandStart(VARIANT_TRUE));
			THROW_IF_FAILED(setting->put_StartWhenAvailable(VARIANT_FALSE));
			THROW_IF_FAILED(setting->put_MultipleInstances(TASK_INSTANCES_PARALLEL));
		}

		{
			com_ptr<IExecAction> execAction{ nullptr };
			{
				com_ptr<IAction> action{ nullptr };
				{
					com_ptr<IActionCollection> actionColl{ nullptr };
					THROW_IF_FAILED(taskDefinition->get_Actions(&actionColl));
					THROW_IF_FAILED(actionColl->Create(TASK_ACTION_EXEC, &action));
				}
				action.query_to(&execAction);
			}

			WCHAR modulePath[MAX_PATH + 1]{};
			RETURN_LAST_ERROR_IF(!GetModuleFileName(wil::GetModuleInstanceHandle(), modulePath, MAX_PATH));

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

		com_ptr<ITriggerCollection> triggerColl{ nullptr };
		THROW_IF_FAILED(taskDefinition->get_Triggers(&triggerColl));

		com_ptr<ITrigger> trigger{ nullptr };
		THROW_IF_FAILED(triggerColl->Create(TASK_TRIGGER_LOGON, &trigger));

		com_ptr<IRegisteredTask> registeredTask{ nullptr };
		BSTR name
		{
#ifdef _WIN64
			const_cast<BSTR>(L"TranslucentFlyouts.Win32 Autorun Task")
#else
			const_cast<BSTR>(L"TranslucentFlyouts.Win32 Autorun Task (x86)")
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
				&registeredTask
			)
		);
	}

	auto create_registry_folder_with_tip = [](std::wstring_view folder, std::wstring_view tip)
	{
		auto key{ wil::reg::create_unique_key(HKEY_CURRENT_USER, folder.data(), wil::reg::key_access::readwrite)};
		wil::reg::set_value_string(key.get(), nullptr, tip.data());
	};
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts", L"Create these registry items manually according to your need, therefore using GUI is a good choice");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\Menu", L"Define your menu appearance");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\Menu\\Animation", L"Define your menu animation");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\Menu\\DisabledHot", L"CustomRendering part of menu");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\Menu\\Focusing", L"CustomRendering part of menu");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\Menu\\Hot", L"CustomRendering part of menu");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\Menu\\Separator", L"CustomRendering part of menu");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\Menu\\DisabledList", L"Menu functionalities is disabled in one of these process");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\Tooltip", L"Define your tooltip appearance");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\Tooltip\\DisabledList", L"Tooltip functionalities is disabled in one of these process");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\DropDown", L"Define your dropdown appearance");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\DropDown\\Animation", L"Define your dropdown animation");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\DropDown\\DisabledList", L"DropDown functionalities is disabled in one of these process");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\BlockList", L"TranslucentFlyouts' s module won't be loaded into one of these process");
	create_registry_folder_with_tip(L"SOFTWARE\\TranslucentFlyouts\\DisabledList", L"TranslucentFlyouts is disabled in one of these process");

	return S_OK;
}
CATCH_LOG_RETURN_HR(wil::ResultFromCaughtException())

HRESULT Application::UninstallApp() try
{
	using namespace wil;
	using namespace TranslucentFlyouts;

	HRESULT hr{ S_OK };
	auto CleanUp{ Utils::RoInit(&hr) };
	THROW_IF_FAILED(hr);

	if (
		ShellMessageBoxW(
			wil::GetModuleInstanceHandle(),
			nullptr,
			Utils::GetResWString<IDS_STRING105>().c_str(),
			nullptr,
			MB_ICONINFORMATION | MB_YESNO
		) == IDYES
		)
	{
		SHDeleteKeyW(
			HKEY_LOCAL_MACHINE, L"SOFTWARE\\TranslucentFlyouts"
		);
		SHDeleteKeyW(
			HKEY_CURRENT_USER, L"SOFTWARE\\TranslucentFlyouts"
		);
#ifdef _WIN64
		SHDeleteKeyW(
			HKEY_LOCAL_MACHINE, L"SOFTWARE\\TranslucentFlyouts_Internals"
		);
		SHDeleteKeyW(
			HKEY_CURRENT_USER, L"SOFTWARE\\TranslucentFlyouts_Internals"
		);
#else
		SHDeleteKeyW(
			HKEY_LOCAL_MACHINE, L"SOFTWARE\\TranslucentFlyouts_Internals(x86)"
		);
		SHDeleteKeyW(
			HKEY_CURRENT_USER, L"SOFTWARE\\TranslucentFlyouts_Internals(x86)"
		);
#endif // _WIN64
	}
	{
		com_ptr<ITaskService> taskService{ wil::CoCreateInstance<ITaskService>(CLSID_TaskScheduler) };
		THROW_IF_FAILED(taskService->Connect(_variant_t{}, _variant_t{}, _variant_t{}, _variant_t{}));

		com_ptr<ITaskFolder> rootFolder{ nullptr };
		THROW_IF_FAILED(taskService->GetFolder(_bstr_t("\\"), &rootFolder));

		BSTR name
		{
#ifdef _WIN64
			const_cast<BSTR>(L"TranslucentFlyouts.Win32 Autorun Task")
#else
			const_cast<BSTR>(L"TranslucentFlyouts.Win32 Autorun Task (x86)")
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