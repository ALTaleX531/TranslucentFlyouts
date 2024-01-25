#include "pch.h"
#include "resource.h"
#include "Utils.hpp"
#include "ApiEx.hpp"
#include "RegHelper.hpp"
#include "HookHelper.hpp"
#include "SymbolResolver.hpp"
#include "MenuHooks.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::MenuHooks
{
	using namespace std::literals;
	constexpr std::array g_hookModuleList
	{
		L"explorer.exe"sv,
		L"Narrator.exe"sv,
		L"minecraft.exe"sv,				// Minecraft Launcher
		L"MusNotifyIcon.exe"sv,
		L"ApplicationFrame.dll"sv,
		L"ExplorerFrame.dll"sv,
		L"InputSwitch.dll"sv,
		L"pnidui.dll"sv,
		L"SecurityHealthSSO.dll"sv,
		L"shell32.dll"sv,
		L"SndVolSSO.dll"sv,
		L"twinui.dll"sv,
		L"twinui.pcshell.dll"sv,
		L"bthprops.cpl"sv,
		// Windows 11
		L"Taskmgr.exe"sv,
		L"museuxdocked.dll"sv,
		L"SecurityHealthSsoUdk.dll"sv,
		L"Taskbar.dll"sv,
		L"Windows.UI.FileExplorer.dll"sv,
		L"Windows.UI.FileExplorer.WASDK.dll"sv,
		L"stobject.dll"sv,
		// Third-party apps
		L"StartIsBack64.dll"sv,
		L"StartIsBack32.dll"sv
	};

	BOOL WINAPI MySetMenuInfo(
		HMENU hMenu,
		LPMENUINFO menuInfo
	);
	HRESULT WINAPI MyGetBackgroundColorForAppUserModelId(
		PCWSTR pszItem, 
		COLORREF* color
	);

	wil::srwlock g_lock{};
	bool g_menuHooksEnabled{ false };
	bool g_iconHooksEnabled{ false };

#pragma data_seg(".shared")
	HookHelper::OffsetStorage g_GetBackgroundColorForAppUserModelId_Offset{ 0 };
	HookHelper::OffsetStorage g_CreateStoreIcon_FlagsData_Offset{ 0 };
#pragma data_seg()
#pragma comment(linker,"/SECTION:.shared,RWS")
#ifdef _WIN64
	BYTE g_CreateStoreIcon_FlagsOpCode[]
	{
		0x41, 0xB8,							// mov r8d, 20000004h
		0x04, 0x00, 0x00, 0x20,
		0x48, 0x8B, 0xD7					// mov rdx, rdi
	};
	constexpr DWORD64 g_opCodeFlagsDataOffset{ 2 };
#else
	BYTE g_CreateStoreIcon_FlagsOpCode[]
	{
		0x68,								// push 20000004h
		0x04, 0x00, 0x00, 0x20,
		0xFF, 0X75, 0x0C					// push dword ptr [ebp+0Ch]
	};
	constexpr DWORD64 g_opCodeFlagsDataOffset{ 1 };
#endif // _WIN64

	PVOID g_cookie{nullptr};
	PVOID g_actualGetBackgroundColorForAppUserModelId{nullptr};
	std::unordered_map<PVOID, std::pair<PVOID, std::optional<HMODULE>>> g_hookTable{};

	void HookAttach(PVOID baseAddress, bool attach);
	void HookAttachAll();
	void HookDetachAll();
	VOID CALLBACK LdrDllNotification(
		ULONG notificationReason,
		HookHelper::PCLDR_DLL_NOTIFICATION_DATA notificationData,
		PVOID context
	);
}

BOOL WINAPI MenuHooks::MySetMenuInfo(
	HMENU hMenu,
	LPMENUINFO menuInfo
)
{
	HMODULE callerModule{ DetourGetContainingModule(_ReturnAddress()) };
	if (!callerModule) [[unlikely]]
	{
		return SetMenuInfo(hMenu, menuInfo);
	}

	if (menuInfo && (menuInfo->fMask & MIM_BACKGROUND) && menuInfo->hbrBack)
	{
		g_targetModule = callerModule;
	}

	return reinterpret_cast<decltype(&MySetMenuInfo)>(std::get<0>(g_hookTable[reinterpret_cast<PVOID>(callerModule)]))(hMenu, menuInfo);
}
HRESULT WINAPI MenuHooks::MyGetBackgroundColorForAppUserModelId(
	PCWSTR pszItem, 
	COLORREF* color
)
{
	if (color)
	{
		*color = 0;
	}
	return E_FAIL;
}

void MenuHooks::Prepare()
{
	auto IsAPIOffsetReady = [&]()
	{
		return g_GetBackgroundColorForAppUserModelId_Offset.IsValid();
	};

	if (IsAPIOffsetReady())
	{
		return;
	}

	if (!RegHelper::Get<DWORD>({ L"Menu" }, L"NoModernAppBackgroundColor", 1))
	{
		return;
	}

	HookHelper::OffsetStorage CreateStoreIcon_Offset{ 0 };

	auto shell32Module{ wil::unique_hmodule{LoadLibraryW(L"shell32.dll")} };
	auto currentVersion{ Utils::GetVersion(shell32Module.get()) };
	auto savedVersion{ RegHelper::__TryGet<std::wstring>({}, L"ShellVersion")};
	if (savedVersion && savedVersion.value() == currentVersion)
	{
		g_CreateStoreIcon_FlagsData_Offset.value = RegHelper::__Get<DWORD64>({}, L"CreateStoreIconFlagsData_Offset", 0);
		g_GetBackgroundColorForAppUserModelId_Offset.value = RegHelper::__Get<DWORD64>({}, L"GetBackgroundColorForAppUserModelId_Offset", 0);
	}

	while (!IsAPIOffsetReady())
	{
		HRESULT hr{ S_OK };
		SymbolResolver symbolResolver{ L"MenuHooks" };
		hr = symbolResolver.Walk(L"shell32.dll", "*!*", [&](PSYMBOL_INFO symInfo, ULONG symbolSize) -> bool
		{
			auto functionName{ reinterpret_cast<const CHAR*>(symInfo->Name) };
			CHAR unDecoratedFunctionName[MAX_PATH + 1]{};
			UnDecorateSymbolName(
				functionName, unDecoratedFunctionName, MAX_PATH,
				UNDNAME_COMPLETE | UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_THROW_SIGNATURES
			);
			CHAR fullyUnDecoratedFunctionName[MAX_PATH + 1]{};
			UnDecorateSymbolName(
				functionName, fullyUnDecoratedFunctionName, MAX_PATH,
				UNDNAME_NAME_ONLY
			);
			auto functionOffset{ HookHelper::OffsetStorage::From(symInfo->ModBase, symInfo->Address) };

			if (!strcmp(fullyUnDecoratedFunctionName, "GetBackgroundColorForAppUserModelId"))
			{
				g_GetBackgroundColorForAppUserModelId_Offset = functionOffset;
			}
			if (!strcmp(fullyUnDecoratedFunctionName, "CreateStoreIcon") || !strcmp(fullyUnDecoratedFunctionName, "_CreateStoreIcon@12"))
			{
				CreateStoreIcon_Offset = functionOffset;

				auto actualCreateStoreIcon{ CreateStoreIcon_Offset.To(shell32Module.get())};
				auto functionBytes{ reinterpret_cast<UCHAR*>(actualCreateStoreIcon) };
				for (int i = 0; i < 65535; i++)
				{
					if (!memcmp(&functionBytes[i], g_CreateStoreIcon_FlagsOpCode, sizeof(g_CreateStoreIcon_FlagsOpCode)))
					{
						g_CreateStoreIcon_FlagsData_Offset = HookHelper::OffsetStorage::From(
							shell32Module.get(),
							&functionBytes[i + g_opCodeFlagsDataOffset]
						);
					}
				}
			}

			if (IsAPIOffsetReady() && CreateStoreIcon_Offset.IsValid())
			{
				return false;
			}

			return true;
		});
		if (FAILED(hr))
		{
			Api::InteractiveIO::OutputToConsole(
				Api::InteractiveIO::StringType::Error,
				Api::InteractiveIO::WaitType::NoWait,
				IDS_STRING103,
				std::format(L"[MenuHooks] "),
				std::format(L"(hresult: {:#08x})\n", static_cast<ULONG>(hr))
			);
		}

		if (IsAPIOffsetReady())
		{
			if (symbolResolver.IsLoaded() && symbolResolver.IsInternetRequired())
			{
				std::filesystem::remove_all(Utils::make_current_folder_file_str(L"symbols"));
				Api::InteractiveIO::OutputToConsole(
					Api::InteractiveIO::StringType::Notification,
					Api::InteractiveIO::WaitType::NoWait,
					IDS_STRING101,
					std::format(L"[MenuHooks] "),
					L"\n"
				);
			}
		}
		else
		{
			if (GetConsoleWindow())
			{
				if (symbolResolver.IsLoaded())
				{
					Api::InteractiveIO::OutputToConsole(
						Api::InteractiveIO::StringType::Error,
						Api::InteractiveIO::WaitType::NoWait,
						IDS_STRING107,
						std::format(L"[MenuHooks] "),
						L"\n"
					);
					break;
				}
				else
				{
					if (
						Api::InteractiveIO::OutputToConsole(
							Api::InteractiveIO::StringType::Warning,
							Api::InteractiveIO::WaitType::WaitYN,
							IDS_STRING102,
							std::format(L"[MenuHooks] "),
							L"\n"
						)
						)
					{
						continue;
					}
					else
					{
						break;
					}
				}
			}
			else
			{
				Api::InteractiveIO::OutputToConsole(
					Api::InteractiveIO::StringType::Error,
					Api::InteractiveIO::WaitType::NoWait,
					IDS_STRING107,
					std::format(L"[MenuHooks] "),
					L"\n"
				);
				break;
			}
		}
	}

	if (IsAPIOffsetReady())
	{
		Api::InteractiveIO::OutputToConsole(
			Api::InteractiveIO::StringType::Notification,
			Api::InteractiveIO::WaitType::NoWait,
			IDS_STRING108,
			std::format(L"[MenuHooks] "),
			std::format(L" ({})\n", currentVersion),
			true
		);
		RegHelper::__Set({}, L"ShellVersion", currentVersion.c_str());
		if (g_CreateStoreIcon_FlagsData_Offset.IsValid())
		{
			RegHelper::__Set<DWORD64>({}, L"CreateStoreIconFlagsData_Offset", g_CreateStoreIcon_FlagsData_Offset.value);
		}
		RegHelper::__Set<DWORD64>({}, L"GetBackgroundColorForAppUserModelId_Offset", g_GetBackgroundColorForAppUserModelId_Offset.value);
	}

	Api::InteractiveIO::OutputToConsole(
		Api::InteractiveIO::StringType::Notification,
		Api::InteractiveIO::WaitType::NoWait,
		0,
		std::format(L"[MenuHooks] "),
		std::format(L"Over. \n"),
		true
	);
}
void MenuHooks::Startup()
{
	g_actualGetBackgroundColorForAppUserModelId = g_GetBackgroundColorForAppUserModelId_Offset.To(GetModuleHandleW(L"shell32.dll"));
}
void MenuHooks::Shutdown()
{
	DisableHooks();
}

void MenuHooks::EnableHooks(bool enable)
{
	auto lock{ g_lock.lock_exclusive() };

	if (enable && !g_menuHooksEnabled)
	{
		HookHelper::g_actualLdrRegisterDllNotification(0, LdrDllNotification, nullptr, &g_cookie);

		HookAttachAll();

		g_menuHooksEnabled = true;
	}
	if (!enable && g_menuHooksEnabled)
	{
		HookDetachAll();

		HookHelper::g_actualLdrUnregisterDllNotification(g_cookie);
		g_cookie = nullptr;

		g_menuHooksEnabled = false;
	}
}

void MenuHooks::EnableIconHooks(bool enable)
{
	auto lock{ g_lock.lock_exclusive() };

	const auto flagsDataAddress{ g_CreateStoreIcon_FlagsData_Offset.To(GetModuleHandleW(L"shell32.dll"))};

	if (enable && !g_iconHooksEnabled)
	{
		if (flagsDataAddress)
		{
			HookHelper::WriteMemory(flagsDataAddress, [&]()
			{
				DWORD flags{ SIIGBF_BIGGERSIZEOK };
				memcpy_s(flagsDataAddress, sizeof(flags), &flags, sizeof(flags));
			});
		}

		if (g_actualGetBackgroundColorForAppUserModelId)
		{
			HRESULT hr = HookHelper::Detours::Write([&]()
			{

				HookHelper::Detours::Attach(&g_actualGetBackgroundColorForAppUserModelId, MenuHooks::MyGetBackgroundColorForAppUserModelId);
			});

			g_iconHooksEnabled = enable;
		}
	}
	if (!enable && g_iconHooksEnabled)
	{
		if (flagsDataAddress)
		{
			HookHelper::WriteMemory(flagsDataAddress, [&]()
			{
				DWORD flags{ 0x20'00'00'04 };
				memcpy_s(flagsDataAddress, sizeof(flags), &flags, sizeof(flags));
			});
		}

		if (g_actualGetBackgroundColorForAppUserModelId)
		{
			HookHelper::Detours::Write([&]()
			{
				HookHelper::Detours::Detach(&g_actualGetBackgroundColorForAppUserModelId, MenuHooks::MyGetBackgroundColorForAppUserModelId);
			});

			g_iconHooksEnabled = enable;
		}
	}
}

void MenuHooks::DisableHooks()
{
	EnableHooks(false);
	EnableIconHooks(false);
}

void MenuHooks::HookAttach(PVOID baseAddress, bool attach)
{
	if (attach)
	{
		auto originalFunction{ HookHelper::WriteIAT(baseAddress, "user32.dll", "SetMenuInfo", MenuHooks::MySetMenuInfo) };
		if (originalFunction)
		{
			g_hookTable[baseAddress] = std::make_pair(originalFunction, std::nullopt);
		}
		else
		{
			auto [originalModule, originalFunction] {HookHelper::WriteDelayloadIAT(baseAddress, "user32.dll", "SetMenuInfo", MenuHooks::MySetMenuInfo)};
			g_hookTable[baseAddress] = std::make_pair(originalFunction, originalModule);
		}
	}
	else
	{
		auto& [originalFunction, originalModule] = g_hookTable[baseAddress];
		if (originalModule)
		{
			HookHelper::WriteDelayloadIAT(baseAddress, "user32.dll", "SetMenuInfo", originalFunction, originalModule);
		}
		else
		{
			HookHelper::WriteIAT(baseAddress, "user32.dll", "SetMenuInfo", originalFunction);
		}
		g_hookTable.erase(baseAddress);
	}
}
void MenuHooks::HookAttachAll()
{
	for (auto moduleName : g_hookModuleList)
	{
		HMODULE moduleHandle{GetModuleHandleW(moduleName.data())};
		if (moduleHandle)
		{
			HookAttach(reinterpret_cast<PVOID>(moduleHandle), true);
		}
	}
}
void MenuHooks::HookDetachAll()
{
	for (auto& [baseAddress, hookInfo] : g_hookTable)
	{
		auto& [originalFunction, originalModule] = hookInfo;
		if (originalModule)
		{
			HookHelper::WriteDelayloadIAT(baseAddress, "user32.dll", "SetMenuInfo", originalFunction, originalModule);
		}
		else
		{
			HookHelper::WriteIAT(baseAddress, "user32.dll", "SetMenuInfo", originalFunction);
		}
	}
	g_hookTable.clear();
}
VOID CALLBACK MenuHooks::LdrDllNotification(
	ULONG notificationReason,
	HookHelper::PCLDR_DLL_NOTIFICATION_DATA notificationData,
	PVOID context
)
{
	if (notificationReason == HookHelper::LDR_DLL_NOTIFICATION_REASON_LOADED)
	{
		auto lock{ g_lock.lock_exclusive() };

		if (g_iconHooksEnabled)
		{
			for (auto moduleName : g_hookModuleList)
			{
				if (!_wcsicmp(moduleName.data(), notificationData->Loaded.BaseDllName->Buffer))
				{
					HookAttach(notificationData->Loaded.DllBase, true);
				}
			}
		}

	}
	if (notificationReason == HookHelper::LDR_DLL_NOTIFICATION_REASON_UNLOADED)
	{
		g_hookTable.erase(notificationData->Unloaded.DllBase);
	}
}