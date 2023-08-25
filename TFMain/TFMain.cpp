#include "pch.h"
#include "resource.h"
#include "RegHelper.hpp"
#include "Utils.hpp"
#include "Hooking.hpp"
#include "ThemeHelper.hpp"
#include "EffectHelper.hpp"
#include "MenuHandler.hpp"
#include "SharedUxTheme.hpp"
#include "ToolTipHandler.hpp"
#include "UxThemePatcher.hpp"
#include "ImmersiveContextMenuPatcher.hpp"
#include "TFMain.hpp"

using namespace TranslucentFlyouts;
using namespace std;

namespace TranslucentFlyouts::TFMain
{
	bool g_debug{ false };
	bool g_startup{ false };
	vector<Callback> g_callbackList{};
}

TFMain::InteractiveIO::~InteractiveIO()
{
	Shutdown();
}

bool TFMain::InteractiveIO::OutputString(
	StringType strType,
	WaitType waitType,
	UINT strResourceId,
	std::wstring_view prefixStr,
	std::wstring_view additionalStr,
	bool requireConsole
) const
{
	if (!requireConsole)
	{
		Startup();
	}
	else if (!GetConsoleWindow())
	{
		return false;
	}

	WCHAR str[MAX_PATH + 1] {};
	LoadStringW(HINST_THISCOMPONENT, strResourceId, str, MAX_PATH);

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
	wcout << prefixStr << str << additionalStr;

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
				input = getchar();
				if (input == 'Y' || input == 'y')
				{
					return true;
				}
				if (input == 'N' || input == 'n')
				{
					return false;
				}
			}
			while (true);
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

void TFMain::InteractiveIO::Startup()
{
	if (GetConsoleWindow() || AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
	{
		FILE* fpstdin{ stdin }, * fpstdout{ stdout }, * fpstderr{ stderr };
		freopen_s(&fpstdin, "CONIN$", "r", stdin);
		freopen_s(&fpstdout, "CONOUT$", "w", stdout);
		freopen_s(&fpstderr, "CONOUT$", "w", stderr);
		_wsetlocale(LC_ALL, L"chs");
	}
}

void TFMain::InteractiveIO::Shutdown()
{
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	FreeConsole();
}

void TFMain::Startup()
{
	if (g_startup)
	{
		return;
	}

	wil::SetResultLoggingCallback([](wil::FailureInfo const & failure) noexcept
	{
		WCHAR logString[MAX_PATH + 1] {};
		SecureZeroMemory(logString, _countof(logString));
		if (SUCCEEDED(wil::GetFailureLogString(logString, _countof(logString), failure)))
		{
			//OutputDebugStringW(logString);

			//auto folder{Utils::make_current_folder_file_str(L"Logs\\")};
			//if (SUCCEEDED(wil::CreateDirectoryDeepNoThrow(folder.c_str())))
			//{
			//	wofstream logFile{folder + Utils::get_module_base_file_name(nullptr) + L".log", ios_base::app};
			//	if (logFile)
			//	{
			//		logFile << logString << endl;
			//		//logFile << logString << L"\n";
			//	}
			//}
		}
	});

	MenuHandler::Startup();
	SharedUxTheme::Startup();
	UxThemePatcher::Startup();
	ImmersiveContextMenuPatcher::Startup();
	ToolTipHandler::Startup();

	g_startup = true;
}
void TFMain::Shutdown()
{
	if (!g_startup)
	{
		return;
	}

	ImmersiveContextMenuPatcher::Shutdown();
	UxThemePatcher::Shutdown();
	SharedUxTheme::Shutdown();
	MenuHandler::Shutdown();
	ToolTipHandler::Shutdown();

	g_startup = false;
}

void TFMain::Prepare()
{
	TFMain::InteractiveIO io{};

	UxThemePatcher::Prepare(io);
	MenuHandler::Prepare(io);

	io.OutputString(
		TFMain::InteractiveIO::StringType::Notification,
		TFMain::InteractiveIO::WaitType::WaitAnyKey,
		IDS_STRING104,
		L"\n"sv,
		L"\n"sv,
		true
	);
}

void CALLBACK TFMain::HandleWinEvent(
	HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hWnd,
	LONG idObject, LONG idChild,
	DWORD dwEventThread, DWORD dwmsEventTime
)
{
	if (
		auto result{ wil::reg::try_get_value_dword(HKEY_CURRENT_USER, L"SOFTWARE\\StartIsBack", L"Disabled") };
		idObject != OBJID_WINDOW ||
		idChild != CHILDID_SELF ||
		!hWnd || !IsWindow(hWnd) ||
		(
			GetModuleHandleW(L"explorer.exe") &&
			GetModuleHandleW(L"StartAllBackX64.dll") &&
			(!result.has_value() || !result.value())
		)
	)
	{
		return;
	}

	if (
		Utils::IsValidFlyout(hWnd) &&
		//ThemeHelper::IsThemeAvailable() &&
		//!ThemeHelper::IsHighContrast() &&
		!GetSystemMetrics(SM_CLEANBOOT)
	)
	{
		TFMain::Startup();
	}

	if (!g_callbackList.empty())
	{
		for (const auto& callback : g_callbackList)
		{
			if (callback)
			{
				callback(hWnd, dwEvent);
			}
		}
	}
}

void TFMain::AddCallback(Callback callback)
{
	g_callbackList.push_back(callback);
}

void TFMain::DeleteCallback(Callback callback)
{
	for (auto it = g_callbackList.begin(); it != g_callbackList.end();)
	{
		auto& callback{*it};
		if (*callback.target<void(HWND, DWORD)>() == *callback.target<void(HWND, DWORD)>())
		{
			it = g_callbackList.erase(it);
			break;
		}
		else
		{
			it++;
		}
	}
}

void TFMain::ApplyBackdropEffect(wstring_view keyName, HWND hWnd, bool darkMode, DWORD darkMode_GradientColor, DWORD lightMode_GradientColor)
{
	DWORD effectType
	{
		RegHelper::GetDword(
			keyName,
			L"EffectType",
			static_cast<DWORD>(EffectHelper::EffectType::ModernAcrylicBlur)
		)
	};
	DWORD enableDropShadow
	{
		RegHelper::GetDword(
			keyName,
			L"EnableDropShadow",
			0
		)
	};
	// Set effect for the popup menu
	DWORD gradientColor{ 0 };
	if (darkMode)
	{
		gradientColor = RegHelper::GetDword(
							keyName,
							L"DarkMode_GradientColor",
							darkMode_GradientColor
						);

		EffectHelper::EnableWindowDarkMode(hWnd, TRUE);
	}
	else
	{
		gradientColor = RegHelper::GetDword(
							keyName,
							L"LightMode_GradientColor",
							lightMode_GradientColor
						);

	}

	EffectHelper::SetWindowBackdrop(hWnd, enableDropShadow, Utils::MakeCOLORREF(gradientColor) | (to_integer<DWORD>(Utils::GetAlpha(gradientColor)) << 24), effectType);
	DwmTransitionOwnedWindow(hWnd, DWMTRANSITION_OWNEDWINDOW_REPOSITION);
}


HRESULT TFMain::ApplyRoundCorners(std::wstring_view keyName, HWND hWnd)
{
	DWORD cornerType
	{
		RegHelper::GetDword(
			keyName,
			L"CornerType",
			3
		)
	};
	if (cornerType != 0)
	{
		return DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerType, sizeof(DWM_WINDOW_CORNER_PREFERENCE));
	}
	return S_OK;
}

HRESULT TFMain::ApplySysBorderColors(std::wstring_view keyName, HWND hWnd, bool useDarkMode, DWORD darkMode_BorderColor, DWORD lightMode_BorderColor)
{
	DWORD noBorderColor
	{
		RegHelper::GetDword(
			keyName,
			L"NoBorderColor",
			0
		)
	};

	DWORD borderColor{ useDarkMode ? darkMode_BorderColor : lightMode_BorderColor };
	if (!noBorderColor)
	{
		DWORD enableThemeColorization
		{
			RegHelper::GetDword(
				keyName,
				L"EnableThemeColorization",
				0
			)
		};

		if (enableThemeColorization)
		{
			LOG_IF_FAILED(Utils::GetDwmThemeColor(borderColor));
		}

		if (useDarkMode)
		{
			borderColor = RegHelper::GetDword(
							  keyName,
							  L"DarkMode_BorderColor",
							  borderColor
						  );
		}
		else
		{
			borderColor = RegHelper::GetDword(
							  keyName,
							  L"LightMode_BorderColor",
							  borderColor
						  );
		}

		borderColor = Utils::MakeCOLORREF(borderColor);
	}
	else
	{
		borderColor = DWMWA_COLOR_NONE;
	}

	return DwmSetWindowAttribute(hWnd, DWMWA_BORDER_COLOR, &borderColor, sizeof(borderColor));
}