#include "pch.h"
#include "resource.h"
#include "Api.hpp"
#include "Utils.hpp"
#include "RegHelper.hpp"
#include "Framework.hpp"
#include "HookHelper.hpp"
#include "Application.hpp"
#include "CommonFlyoutsHandler.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::Framework
{
	constexpr std::wstring_view messageWindowClassName{ L"TranslucentFlyouts.Immersive.MessageWindow" };
	const UINT TFM_SHUTDOWN{ RegisterWindowMessageW(L"TranslucentFlyouts.Immersive.ShutdownMessageWindow") };
	const UINT TFM_UPDATE{ RegisterWindowMessageW(L"TranslucentFlyouts.Immersive.Update") };
	const auto g_currentDllPath{ wil::GetModuleFileNameW<std::wstring, MAX_PATH + 1>(wil::GetModuleInstanceHandle()) };
	wil::unique_registry_watcher g_registryWatcher{ nullptr };
	wil::unique_handle g_messageThread{ nullptr };
	HWND g_messageWindow{ nullptr };

	HMODULE Inject(HANDLE processHandle);
	void OnRegistryItemsChanged();
	void WalkMessageWindows(const std::function<bool(HWND)>&& callback);

	void Update();
	DWORD WINAPI MessageThreadProc(LPVOID);
	void DoExplorerCrashCheck();
}

void Framework::DoExplorerCrashCheck()
{
	static std::chrono::steady_clock::time_point g_lastExplorerDied{ std::chrono::steady_clock::time_point{} - std::chrono::seconds(30) };
	static std::chrono::steady_clock::time_point g_lastExplorerDied2{ std::chrono::steady_clock::time_point{} - std::chrono::seconds(10) };
	static DWORD g_lastExplorerPid
	{
		[]
		{
			DWORD explorerPid{ 0 };
			GetWindowThreadProcessId(GetShellWindow(), &explorerPid);

			return explorerPid;
		} ()
	};

	auto terminate = [](UINT id)
	{
		Application::UninstallHook();

		static WCHAR msg[32768 + 1]{};
		LoadStringW(wil::GetModuleInstanceHandle(), id, msg, 32768);
		MessageBoxW(
			nullptr,
			msg,
			nullptr,
			MB_ICONERROR | MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION | MB_SETFOREGROUND
		);

		std::thread{ [&]
		{
			Application::StopService();
		} }.detach();
	};

	DWORD explorerPid{ 0 };
	GetWindowThreadProcessId(GetShellWindow(), &explorerPid);

	if (explorerPid)
	{
		g_lastExplorerDied2 = std::chrono::steady_clock::now();
		g_lastExplorerPid = explorerPid;
	}

	// Being dead for too long!
	{
		const auto currentTimePoint{ std::chrono::steady_clock::now() };
		if (currentTimePoint >= g_lastExplorerDied2 + std::chrono::seconds(5))
		{
			terminate(IDS_STRING103);
			return;
		}
	}
	// Died twice in a short time!
	if (g_lastExplorerPid && explorerPid == 0)
	{
		const auto currentTimePoint{ std::chrono::steady_clock::now() };

		if (currentTimePoint < g_lastExplorerDied + std::chrono::seconds(30)) [[unlikely]]
		{
			terminate(IDS_STRING102);
			return;
		}

		g_lastExplorerDied = currentTimePoint;
		g_lastExplorerDied2 = currentTimePoint;
		g_lastExplorerPid = 0;
	}
}

DWORD WINAPI Framework::MessageThreadProc(LPVOID)
{
	SetThreadDescription(GetCurrentThread(), L"TranslucentFlyouts.Immersive Message Thread");
	{
		auto WndProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT
		{
			if (uMsg == WM_DESTROY)
			{
				PostQuitMessage(0);
			}
			if (uMsg == TFM_SHUTDOWN)
			{
				DestroyWindow(hWnd);
			}
			if (uMsg == TFM_UPDATE)
			{
				Update();
			}

			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		};
		WNDCLASSEX wcex
		{
			.cbSize{ sizeof(WNDCLASSEX) },
			.lpfnWndProc{ WndProc },
			.hInstance{ wil::GetModuleInstanceHandle() },
			.lpszClassName{ messageWindowClassName.data() }
		};
		ATOM classAtom{ RegisterClassExW(&wcex) };
		auto cleanUp = wil::scope_exit([&]
		{
			g_messageThread.reset();
			if (classAtom)
			{
				UnregisterClassW(MAKEINTRESOURCEW(classAtom), wil::GetModuleInstanceHandle());
				classAtom = 0;
			}
		});
		if (!classAtom)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		g_messageWindow = CreateWindowExW(
			WS_EX_NOREDIRECTIONBITMAP | WS_EX_TOOLWINDOW,
			wcex.lpszClassName, wcex.lpszClassName,
			WS_POPUP,
			0,
			0,
			0,
			0,
			HWND_MESSAGE, nullptr, wil::GetModuleInstanceHandle(), nullptr
		);
		if (!g_messageWindow)
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		MSG msg{};
		while (GetMessageW(&msg, g_messageWindow, 0, 0)) { DispatchMessageW(&msg); }
	}
	FreeLibraryAndExitThread(wil::GetModuleInstanceHandle(), 0);
}

HMODULE Framework::Inject(HANDLE processHandle)
{
	PVOID remoteAddress{ nullptr };
	auto cleanUp = wil::scope_exit([&]
	{
		if (remoteAddress)
		{
			VirtualFreeEx(processHandle, remoteAddress, 0x0, MEM_RELEASE);
		}
	});

	CHAR dllPath[MAX_PATH + 1];
	if(!GetModuleFileNameA(wil::GetModuleInstanceHandle(), dllPath, MAX_PATH))
	{
		return nullptr;
	}
	auto dllPathLength{ strlen(dllPath) + 1 };

	remoteAddress = VirtualAllocEx(processHandle, nullptr, dllPathLength, MEM_COMMIT, PAGE_READWRITE);
	if (!remoteAddress)
	{
		return nullptr;
	}

	auto startRoutine = reinterpret_cast<LPTHREAD_START_ROUTINE>(LoadLibraryA);
	if (!WriteProcessMemory(processHandle, remoteAddress, dllPath, dllPathLength, nullptr))
	{
		return nullptr;
	}

	#pragma warning(suppress : 6387)
	wil::unique_handle threadHandle
	{
		CreateRemoteThread(
			processHandle,
			nullptr,
			0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryA")),
			remoteAddress,
			0,
			nullptr
		)
	};
	if (!threadHandle)
	{
		return nullptr;
	}

	WaitForSingleObject(threadHandle.get(), 1000);
	return HookHelper::GetProcessModule(processHandle, g_currentDllPath);
}

void Framework::OnRegistryItemsChanged()
{
	std::optional<DWORD> value{};
	g_immersiveContext = ImmersiveContext{};
	g_immersiveContext.disabled = RegHelper::Get<DWORD>(
		{ L"ImmersiveFlyouts", L""},
		L"Disabled",
		0,
		1
	);
	value = RegHelper::TryGet<DWORD>(
		{ L"ImmersiveFlyouts" },
		L"DarkMode_TintColor"
	);
	if (value.has_value())
	{
		auto bits{ Utils::FromARGB(value.value()) };
		g_immersiveContext.darkMode_TintColor.A = std::get<0>(bits);
		g_immersiveContext.darkMode_TintColor.R = std::get<1>(bits);
		g_immersiveContext.darkMode_TintColor.G = std::get<2>(bits);
		g_immersiveContext.darkMode_TintColor.B = std::get<3>(bits);
	}
	value = RegHelper::TryGet<DWORD>(
		{ L"ImmersiveFlyouts" },
		L"LightMode_TintColor"
	);
	if (value.has_value())
	{
		auto bits{ Utils::FromARGB(value.value()) };
		g_immersiveContext.lightMode_TintColor.A = bits[0];
		g_immersiveContext.lightMode_TintColor.R = bits[1];
		g_immersiveContext.lightMode_TintColor.G = bits[2];
		g_immersiveContext.lightMode_TintColor.B = bits[3];
	}
	value = RegHelper::TryGet<DWORD>(
		{ L"ImmersiveFlyouts" },
		L"DarkMode_LuminosityOpacity"
	);
	if (value.has_value())
	{
		g_immersiveContext.darkMode_LuminosityOpacity = value.value() / 255.f;
	}
	value = RegHelper::TryGet<DWORD>(
		{ L"ImmersiveFlyouts" },
		L"LightMode_LuminosityOpacity"
	);
	if (value.has_value())
	{
		g_immersiveContext.lightMode_LuminosityOpacity = value.value() / 255.f;
	}
	value = RegHelper::TryGet<DWORD>(
		{ L"ImmersiveFlyouts" },
		L"DarkMode_TintOpacity"
	);
	if (value.has_value())
	{
		g_immersiveContext.darkMode_TintOpacity = value.value() / 255.f;
	}
	value = RegHelper::TryGet<DWORD>(
		{ L"ImmersiveFlyouts" },
		L"LightMode_TintOpacity"
	);
	if (value.has_value())
	{
		g_immersiveContext.lightMode_TintOpacity = value.value() / 255.f;
	}
	value = RegHelper::TryGet<DWORD>(
		{ L"ImmersiveFlyouts" },
		L"DarkMode_Opacity"
	);
	if (value.has_value())
	{
		g_immersiveContext.darkMode_Opacity = value.value() / 255.f;
	}
	value = RegHelper::TryGet<DWORD>(
		{ L"ImmersiveFlyouts" },
		L"LightMode_Opacity"
	);
	if (value.has_value())
	{
		g_immersiveContext.lightMode_Opacity = value.value() / 255.f;
	}

	CommonFlyoutsHandler::OnRegistryItemsChanged();
	// update all
	WalkMessageWindows([](HWND hwnd)
	{
		WCHAR className[MAX_PATH + 1]{};
		if (GetClassNameW(hwnd, className, MAX_PATH) && !wcscmp(className, messageWindowClassName.data()))
		{
			SendNotifyMessageW(hwnd, TFM_UPDATE, 0, 0);
		}

		return true;
	});
}

void Framework::Startup()
{
	g_messageThread.reset(
		CreateThread(
			nullptr,
			0,
			MessageThreadProc,
			nullptr,
			0,
			nullptr
		)
	);

	DiagnosticsHandler::Startup();
	CommonFlyoutsHandler::Startup();
}

void Framework::Shutdown()
{
	if (g_messageThread)
	{
		#pragma warning(suppress : 6258)
		TerminateThread(g_messageThread.get(), -1);
		g_messageThread.reset();
	}

	CommonFlyoutsHandler::Shutdown();
	DiagnosticsHandler::Shutdown();
}

void Framework::Update()
{
	CommonFlyoutsHandler::Update();
}

void Framework::OnVisualTreeChanged(IInspectable* element, DiagnosticsHandler::FrameworkType framework, DiagnosticsHandler::MutationType mutation)
{
	CommonFlyoutsHandler::OnVisualTreeChanged(element, framework, mutation);
}

void Framework::Prepare()
{
	try
	{
		static const auto s_pfnRtlAdjustPrivilege{ (NTSTATUS(NTAPI*)(int, BOOLEAN, BOOLEAN, PBOOLEAN))GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlAdjustPrivilege") };
		if (s_pfnRtlAdjustPrivilege) [[likely]]
		{
			BOOLEAN result = false;
			constexpr auto SE_DEBUG_PRIVILEGE{ 0x14 };
			s_pfnRtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, true, false, &result);
		}

		WCHAR dllPath[MAX_PATH + 1]{};
		THROW_LAST_ERROR_IF(GetModuleFileNameW(wil::GetModuleInstanceHandle(), dllPath, MAX_PATH) == 0);

		PSECURITY_DESCRIPTOR securityDescriptor{ nullptr };
		auto cleanUp = wil::scope_exit([&]
		{
			if (securityDescriptor)
			{
				LocalFree(securityDescriptor);
			}
		});
		ULONG securityDescriptorLength{0};
		THROW_IF_WIN32_BOOL_FALSE(
			ConvertStringSecurityDescriptorToSecurityDescriptorA(
				"D:(A;;GRGX;;;S-1-15-2-1)(A;;GRGX;;;S-1-15-2-2)", SDDL_REVISION, &securityDescriptor,
				&securityDescriptorLength
			)
		);

		PACL dacl{ nullptr };
		BOOL present{ FALSE }, defaulted{ FALSE };
		THROW_IF_WIN32_BOOL_FALSE(
			GetSecurityDescriptorDacl(securityDescriptor, &present, &dacl, &defaulted)
		);
		THROW_IF_WIN32_ERROR(
			SetNamedSecurityInfoW(
				dllPath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr,
				nullptr, dacl, nullptr
			)
		);

		g_registryWatcher.create(HKEY_CURRENT_USER, LR"(Software\TranslucentFlyouts\ImmersiveFlyouts)", true, [](wil::RegistryChangeKind changeType)
		{
			OnRegistryItemsChanged();
		});

		DiagnosticsHandler::Prepare();
	}
	catch(...) {}

	OnRegistryItemsChanged();
}

void CALLBACK Framework::HandleWinEvent(
	HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
	LONG idObject, LONG idChild,
	DWORD dwEventThread, DWORD dwmsEventTime
)
{
	Framework::DoExplorerCrashCheck();
	if (Api::IsPartDisabled(L"ImmersiveFlyouts")) [[unlikely]]
	{
		return;
	}

	if (
		Utils::IsWindowClass(hWnd, L"Windows.UI.Core.CoreWindow") || 
		Utils::IsWindowClass(hWnd, L"Windows.UI.Composition.DesktopWindowContentBridge") ||
		Utils::IsWindowClass(hWnd, L"Microsoft.UI.Content.DesktopChildSiteBridge") ||
		Utils::IsWindowClass(hWnd, L"XamlExplorerHostIslandWindow") ||
		Utils::IsWindowClass(hWnd, L"Xaml_WindowedPopupClass")
	)
	{
		DWORD processId{0};
		GetWindowThreadProcessId(hWnd, &processId);

		wil::unique_process_handle processHandle
		{
			OpenProcess(
				PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
				FALSE,
				processId
			)
		};
		WCHAR processName[MAX_PATH + 1]{};
		GetModuleBaseNameW(processHandle.get(), nullptr, processName, MAX_PATH);

		if (
			!HookHelper::GetProcessModule(processHandle.get(), g_currentDllPath) &&
			(
				!_wcsicmp(L"explorer.exe", processName) ||
				!_wcsicmp(L"PhotosApp.exe", processName) ||
				!_wcsicmp(L"Microsoft.Media.Player.exe", processName) ||
				!_wcsicmp(L"StartMenuExperienceHost.exe", processName) ||
				!_wcsicmp(L"ShellExperienceHost.exe", processName)
			)
		)
		{
			Inject(processHandle.get());

#ifdef _DEBUG
			OutputDebugStringW(
				std::format(
					L"[{}] process: {} injected\n",
					processId,
					wil::QueryFullProcessImageNameW<std::wstring, MAX_PATH + 1>(processHandle.get()).c_str()
				).c_str()
			);
#endif
		}
	}
}

void Framework::CleanUp()
{
	g_registryWatcher.reset();
	WalkMessageWindows([](HWND hwnd)
	{
		WCHAR className[MAX_PATH + 1]{};
		if (GetClassNameW(hwnd, className, MAX_PATH) && !wcscmp(className, messageWindowClassName.data()))
		{
			SendNotifyMessageW(hwnd, TFM_SHUTDOWN, 0, 0);
		}

		return true;
	});
}

void Framework::WalkMessageWindows(const std::function<bool(HWND)>&& callback)
{
	HWND hwnd{ nullptr };
	do
	{
		hwnd = FindWindowExW(HWND_MESSAGE, hwnd, nullptr, nullptr);
		if (hwnd)
		{
			if (!callback(hwnd))
			{
				break;
			}
		}
	} while (hwnd);
}