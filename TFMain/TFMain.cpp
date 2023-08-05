#include "pch.h"
#include "resource.h"
#include "TFMain.hpp"

using namespace TranslucentFlyouts;
using namespace std;

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
	L"msedgewebview2.exe"sv
};

#pragma data_seg("hook")
HWINEVENTHOOK MainDLL::g_hHook {nullptr};
#pragma data_seg()
#pragma comment(linker,"/SECTION:hook,RWS")

MainDLL& MainDLL::GetInstance()
{
	static MainDLL instance{};
	return instance;
}

MainDLL::MainDLL()
{
	wil::SetResultLoggingCallback([](wil::FailureInfo const & failure) noexcept
	{
		WCHAR logString[MAX_PATH + 1] {};
		SecureZeroMemory(logString, _countof(logString));
		if (SUCCEEDED(wil::GetFailureLogString(logString, _countof(logString), failure)))
		{
			//OutputDebugStringW(logString);

			/*wil::unique_hfile file
			{
				CreateFile2(
					Utils::make_current_folder_file_str(L"debug.log").c_str(),
					FILE_APPEND_DATA,
					FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
					OPEN_EXISTING,
					nullptr
				)
			};
			if (!file)
			{
				file.reset(
					CreateFile2(
						Utils::make_current_folder_file_str(L"debug.log").c_str(),
						FILE_APPEND_DATA,
						FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
						OPEN_ALWAYS,
						nullptr
					)
				);
				auto header{0xFEFF};
				WriteFile(file.get(), &header, sizeof(header), nullptr, nullptr);
			}

			if (file)
			{
				WriteFile(file.get(), logString, wcslen(logString), nullptr, nullptr);
			}*/
		}
	});
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
				  HandleWinEvent,
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
		WM_POWERBROADCAST,
		PBT_APMRESUMESUSPEND,
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

void MainDLL::Startup()
{
	if (m_startup)
	{
		return;
	}

	m_menuHandler.StartupHook();
	m_uxthemePatcher.StartupHook();
	m_immersiveContextMenuPatcher.StartupHook();

	m_startup = true;
}
void MainDLL::Shutdown()
{
	if (!m_startup)
	{
		return;
	}

	m_immersiveContextMenuPatcher.ShutdownHook();
	m_uxthemePatcher.ShutdownHook();
	m_menuHandler.ShutdownHook();

	m_startup = false;
}

void CALLBACK MainDLL::HandleWinEvent(
	HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
	LONG idObject, LONG idChild,
	DWORD dwEventThread, DWORD dwmsEventTime
)
{
	auto& mainDLL{GetInstance()};

	if (
		!IsHookGlobalInstalled() ||
		idObject != OBJID_WINDOW ||
		idChild != CHILDID_SELF ||
		!hWnd || !IsWindow(hWnd)
	)
	{
		return;
	}

	auto& callbackList{GetInstance().m_callbackList};

	if (!callbackList.empty())
	{
		for (const auto& callback : callbackList)
		{
			if (callback)
			{
				callback(hWnd, dwEvent);
			}
		}
	}
}

void MainDLL::AddCallback(Callback callback)
{
	m_callbackList.push_back(callback);
}

void MainDLL::DeleteCallback(Callback callback)
{
	for (auto it = m_callbackList.begin(); it != m_callbackList.end();)
	{
		auto& callback{*it};
		if (*callback.target<void(HWND, DWORD)>() == *callback.target<void(HWND, DWORD)>())
		{
			it = m_callbackList.erase(it);
			break;
		}
		else
		{
			it++;
		}
	}
}