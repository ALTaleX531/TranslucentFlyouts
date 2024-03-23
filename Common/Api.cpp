#include "Utils.hpp"
#include "RegHelper.hpp"
#include "SystemHelper.hpp"
#include "ThemeHelper.hpp"
#include "Api.hpp"

using namespace TranslucentFlyouts;
namespace TranslucentFlyouts::Api::InteractiveIO
{
	std::function<void()> g_callback{};
}

bool Api::IsServiceRunning(std::wstring_view serviceName)
{
	return wil::unique_handle{ OpenFileMappingFromApp(FILE_MAP_READ, FALSE, serviceName.data()) }.get() != nullptr;
}

bool Api::IsHostProcess(std::wstring_view serviceName)
{
	auto serviceInfo{ Api::GetServiceInfo(serviceName) };

	DWORD processId{0};
	if (serviceInfo && GetWindowThreadProcessId(serviceInfo->hostWindow, &processId) && processId == GetCurrentProcessId())
	{
		return true;
	}

	return false;
}

Api::unique_service_info Api::GetServiceInfo(std::wstring_view serviceName, bool readOnly)
{
	wil::unique_handle fileMapping{ OpenFileMappingW(FILE_MAP_READ | (readOnly ? 0 : FILE_MAP_WRITE), FALSE, serviceName.data())};
	return unique_service_info{ reinterpret_cast<ServiceInfo*>(MapViewOfFileFromApp(fileMapping.get(), FILE_MAP_READ | (readOnly ? 0 : FILE_MAP_WRITE), 0, 0)) };
}
std::pair<wil::unique_handle, Api::unique_service_info> Api::CreateService(std::wstring_view serviceName)
{
	wil::unique_handle fileMapping{ nullptr };
	Api::unique_service_info serviceInfo{ nullptr };

	fileMapping.reset(
		CreateFileMappingFromApp(
			INVALID_HANDLE_VALUE,
			nullptr,
			PAGE_READWRITE | SEC_COMMIT,
			sizeof(Api::ServiceInfo),
			serviceName.data()
		)
	);
	if (!fileMapping)
	{
		return std::make_pair<wil::unique_handle, Api::unique_service_info>(std::move(fileMapping), std::move(serviceInfo));
	}
	serviceInfo.reset(
		reinterpret_cast<Api::ServiceInfo*>(
			MapViewOfFileFromApp(
				fileMapping.get(),
				FILE_MAP_READ | FILE_MAP_WRITE,
				0,
				0
			)
		)
	);

	return std::make_pair<wil::unique_handle, Api::unique_service_info>(std::move(fileMapping), std::move(serviceInfo));
}
bool Api::IsCurrentProcessInBlockList()
{
	using namespace std::literals;
	// TranslucentFlyouts won't be loaded into one of these process
	// These processes are quite annoying because TranslucentFlyouts will not be automatically unloaded by them
	// Some of them even have no chance to show flyouts and other UI elements
	static const std::array blockList
	{
		L"sihost.exe"sv,
		L"WSHost.exe"sv,
		L"spoolsv.exe"sv,
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
		L"Microsoft.SharePoint.exe"sv,
		L"PerfWatson2.exe"sv,
		L"splwow64.exe"sv,
		L"ServiceHub.VSDetouredHost.exe"sv,
		L"CoreWidgetProvider.exe"sv,
		L"FileCoAuth.exe"sv,
		L"WerFault.exe"sv
	};
	auto is_in_list = [](const auto list)
	{
		for (auto item : list)
		{
			auto processInRegistryBlockList{ RegHelper::TryGet<DWORD>({L"BlockList"}, item) };
			if (GetModuleHandleW(item.data()) && !(processInRegistryBlockList.has_value() && !processInRegistryBlockList.value()))
			{
				return true;
			}
		}

		return false;
	};

	return is_in_list(blockList);
}

bool Api::InteractiveIO::OutputToConsole(
	StringType strType,
	WaitType waitType,
	UINT strResourceId,
	std::wstring_view prefixStr,
	std::wstring_view additionalStr,
	bool requireConsole
)
{
	if (!requireConsole)
	{
		Startup();
	}
	else if (!GetConsoleWindow())
	{
		return false;
	}

	LPCWSTR buffer{ nullptr };
	auto length{ LoadStringW(wil::GetModuleInstanceHandle(), strResourceId, reinterpret_cast<LPWSTR>(&buffer), 0)};

	switch (strType)
	{
	case StringType::Notification:
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;
	}
	case StringType::Warning:
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;
	}
	case StringType::Error:
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_INTENSITY);
		break;
	}
	default:
		break;
	}

	auto outputString{std::format(L"{}{}{}", prefixStr, std::wstring_view{ buffer, static_cast<size_t>(length) }, additionalStr)};
	wprintf_s(outputString.c_str());
#ifdef _DEBUG
	OutputDebugStringW(outputString.c_str());
#endif

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY);

	switch (waitType)
	{
	case WaitType::NoWait:
	{
		break;
	}
	case WaitType::WaitYN:
	{
		char input{ };
		do
		{
			input = static_cast<char>(getchar());
			if (input == 'Y' || input == 'y')
			{
				return true;
			}
			if (input == 'N' || input == 'n')
			{
				return false;
			}
		} while (true);
		break;
	}
	case WaitType::WaitAnyKey:
	{
		system("pause>nul");
		break;
	}
	default:
		break;
	}

	return true;
}

void Api::InteractiveIO::Startup()
{
	HWND hwnd{ GetConsoleWindow() };
	if (!hwnd)
	{
		if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
		{
			hwnd = GetConsoleWindow();
			FILE* fpstdin{ stdin }, * fpstdout{ stdout };
			_wfreopen_s(&fpstdin, L"CONIN$", L"r", stdin);
			_wfreopen_s(&fpstdout, L"CONOUT$", L"w+t", stdout);
			_wsetlocale(LC_ALL, L"");

			if (g_callback)
			{
				g_callback();
			}
		}
	}
}

void Api::InteractiveIO::Shutdown()
{
	if (GetConsoleWindow())
	{
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);

		FreeConsole();
	}
}

void Api::InteractiveIO::SetConsoleInitializationCallback(const std::function<void()>&& callback)
{
	g_callback = callback;
}

bool Api::IsPartDisabled(std::wstring_view part)
{
	return IsPartDisabledExternally(part) ||
		RegHelper::Get<DWORD>(
			{ part, L"" },
			L"Disabled",
			0,
			1
	) ||
	RegHelper::Get<DWORD>({ L"DisabledList", part }, Utils::get_process_name(), 0, 1);
}

bool Api::IsPartDisabledExternally(std::wstring_view part)
{
	SYSTEM_POWER_STATUS powerStatus{};

	return (
		IsStartAllBackTakingOver(part) ||
		GetSystemMetrics(SM_CLEANBOOT) ||
		ThemeHelper::IsHighContrast() ||
		(
			GetSystemPowerStatus(&powerStatus) &&
			powerStatus.SystemStatusFlag
		) ||
		RegHelper::Get<DWORD>({ L"DisabledList" }, Utils::get_process_name(), 0)
	);
}

bool Api::IsStartAllBackTakingOver(std::wstring_view part)
{
	if (GetModuleHandleW(L"StartAllBackX64.dll"))
	{
		if (!_wcsicmp(part.data(), L"DropDown"))
		{
			return false;
		}

		if (part.empty() || !_wcsicmp(part.data(), L"Menu"))
		{
			auto immersiveMenus{ wil::reg::try_get_value_dword(HKEY_CURRENT_USER, L"SOFTWARE\\StartIsBack", L"ImmersiveMenus") };
			if (immersiveMenus && immersiveMenus.value() == 0)
			{
				return false;
			}
		}
		if (part.empty() || !_wcsicmp(part.data(), L"Tooltip"))
		{
			auto immersiveTooltips{ wil::reg::try_get_value_dword(HKEY_CURRENT_USER, L"SOFTWARE\\StartIsBack", L"NoDarkTooltips") };
			if (immersiveTooltips && immersiveTooltips.value() == 1)
			{
				return false;
			}
		}

		auto disabled{ wil::reg::try_get_value_dword(HKEY_CURRENT_USER, L"SOFTWARE\\StartIsBack", L"Disabled") };
		if (disabled && disabled.value())
		{
			return false;
		}

		return true;
	}

	return false;
}