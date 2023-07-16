#include "pch.h"
#include "resource.h"
#include "Utils.hpp"
#include "Hooking.hpp"
#include "ThemeHelper.hpp"
#include "EffectHelper.hpp"
#include "MenuHandler.hpp"
#include "UxThemePatcher.hpp"
#include "ImmersiveContextMenuPatcher.hpp"

namespace TranslucentFlyouts
{
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

	// A class that in charge of global injection, initialization and uninitialization
	// of different class which tweak the flyout appearance
	class MainDLL
	{
	public:
		static MainDLL& GetInstance()
		{
			static MainDLL instance{};
			return instance;
		}
		~MainDLL() noexcept = default;
		MainDLL(const MainDLL&) = delete;
		MainDLL& operator=(const MainDLL&) = delete;

		static inline bool IsHookGlobalInstalled()
		{
			return g_hHook != nullptr;
		}
		static HRESULT InstallHook()
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
		static HRESULT UninstallHook()
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

		static bool IsCurrentProcessInBlockList()
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

		void Startup()
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
		void Shutdown()
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
	private:
		MainDLL() = default;
		static void CALLBACK HandleWinEvent(
			HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
			LONG idObject, LONG idChild,
			DWORD dwEventThread, DWORD dwmsEventTime
		)
		{
			auto& mainDLL{GetInstance()};

			if (
				!IsHookGlobalInstalled() ||
				!mainDLL.m_startup ||
				idObject != OBJID_WINDOW ||
				idChild != CHILDID_SELF ||
				!hWnd || !IsWindow(hWnd)
			)
			{
				return;
			}

			if (dwEvent == EVENT_OBJECT_CREATE)
			{
				if (Utils::IsWin32PopupMenu(hWnd))
				{
					GetInstance().m_menuHandler.AttachPopupMenu(hWnd);
				}

				HWND parentWindow{GetParent(hWnd)};
				if (Utils::IsWindowClass(hWnd, L"Listviewpopup") && Utils::IsWindowClass(parentWindow, L"DropDown"))
				{
					GetInstance().m_menuHandler.AttachListViewPopup(hWnd);
				}
			}

			if (dwEvent == EVENT_OBJECT_SHOW)
			{
			}

			if (dwEvent == EVENT_OBJECT_HIDE)
			{
			}
		}

		static HWINEVENTHOOK g_hHook;

		bool m_startup{false};
		MenuHandler& m_menuHandler{MenuHandler::GetInstance()};
		UxThemePatcher& m_uxthemePatcher{UxThemePatcher::GetInstance()};
		ImmersiveContextMenuPatcher& m_immersiveContextMenuPatcher{ImmersiveContextMenuPatcher::GetInstance()};
	};

#pragma data_seg("hook")
	HWINEVENTHOOK MainDLL::g_hHook {nullptr};
#pragma data_seg()
#pragma comment(linker,"/SECTION:hook,RWS")
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
			else
			{
				if (MainDLL::IsHookGlobalInstalled() && ThemeHelper::IsThemeAvailable())
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
			std::format(
				L"/create /sc ONLOGON /tn \"TranslucentFlyouts Autorun Task\" /rl HIGHEST /tr \"Rundll32 \\\"{}\\\",Main\"",
				modulePath
			)
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
			L"/delete /f /tn \"TranslucentFlyouts Autorun Task\"",
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