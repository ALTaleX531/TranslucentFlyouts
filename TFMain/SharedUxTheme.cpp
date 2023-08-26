#include "pch.h"
#include "Hooking.hpp"
#include "RegHelper.hpp"
#include "ThemeHelper.hpp"
#include "MenuHandler.hpp"
#include "ImmersiveContextMenuPatcher.hpp"
#include "ToolTipHandler.hpp"
#include "SharedUxTheme.hpp"
#include "MenuRendering.hpp"

using namespace std;
using namespace TranslucentFlyouts;

namespace TranslucentFlyouts::SharedUxTheme
{
	// A list of modules that need to be hooked multiple times.
	const array g_hookModuleList
	{
		L"explorer.exe"sv,
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
	const array g_delayHookModuleList
	{
		L"explorer.exe"sv
	};

	HRESULT WINAPI DrawThemeBackground(
		HTHEME  hTheme,
		HDC     hdc,
		int     iPartId,
		int     iStateId,
		LPCRECT pRect,
		LPCRECT pClipRect
	);
	void DllNotificationCallback(bool load, Hooking::DllNotifyRoutine::DllInfo info);
	void DoIATHook(PVOID moduleBaseAddress);
	void UndoIATHook(PVOID moduleBaseAddress);

	bool g_startup{ false };
	bool g_disableOnce{false};
	decltype(DrawThemeBackground)* g_actualDrawThemeBackground{ nullptr };
}

HRESULT WINAPI SharedUxTheme::DrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	WCHAR themeClassName[MAX_PATH + 1]{};
	RETURN_IF_FAILED(
		ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)
	);

	if (!_wcsicmp(themeClassName, L"Menu"))
	{
		return ImmersiveContextMenuPatcher::DrawThemeBackground(
			hTheme,
			hdc,
			iPartId,
			iStateId,
			pRect,
			pClipRect
		);
	}

	if (!_wcsicmp(themeClassName, L"Tooltip"))
	{
		return ToolTipHandler::DrawThemeBackground(
			hTheme,
			hdc,
			iPartId,
			iStateId,
			pRect,
			pClipRect
		);
	}

	return g_actualDrawThemeBackground(
		hTheme,
		hdc,
		iPartId,
		iStateId,
		pRect,
		pClipRect
	);
}

HRESULT WINAPI SharedUxTheme::DrawThemeBackgroundHelper(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect,
	bool	darkMode,
	bool&	handled
)
{
	RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hTheme);
	RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
	RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, pRect);
	RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(pRect));
	RETURN_HR_IF_EXPECTED(E_INVALIDARG, IsRectEmpty(pRect) == TRUE);

	WCHAR themeClassName[MAX_PATH + 1]{};
	RETURN_IF_FAILED(
		ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)
	);

	RECT clipRect{ *pRect };
	if (pClipRect)
	{
		IntersectRect(&clipRect, &clipRect, pClipRect);
	}

	if (!_wcsicmp(themeClassName, L"Menu"))
	{
		DWORD customRendering
		{
			RegHelper::GetDword(
				L"Menu",
				L"EnableCustomRendering",
				0,
				false
			)
		};

		// Separator
		if (iPartId == MENU_POPUPSEPARATOR)
		{
			if (customRendering)
			{
				if (SUCCEEDED(MenuRendering::DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
				{
					handled = true;
					return S_OK;
				}
			}
		}
		// Focusing
		if (iPartId == MENU_POPUPITEMKBFOCUS)
		{
			if (customRendering)
			{
				if (SUCCEEDED(MenuRendering::DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
				{
					handled = true;
					return S_OK;
				}
			}
		}

		if ((iPartId == MENU_POPUPITEM || iPartId == MENU_POPUPITEM_FOCUSABLE))
		{
			if (iStateId == MPI_DISABLEDHOT)
			{
				if (customRendering)
				{
					if (SUCCEEDED(MenuRendering::DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
					{
						handled = true;
						return S_OK;
					}
				}
			}
			if (iStateId == MPI_HOT)
			{
				if (customRendering)
				{
					if (SUCCEEDED(MenuRendering::DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
					{
						handled = true;
						return S_OK;
					}
				}

				handled = true;
				return g_actualDrawThemeBackground(
					hTheme,
					hdc,
					iPartId,
					iStateId,
					pRect,
					pClipRect
				);
			}
		}

		{
			RETURN_HR_IF_EXPECTED(
				E_NOTIMPL,
				iPartId != MENU_POPUPBACKGROUND &&
				iPartId != MENU_POPUPBORDERS &&
				iPartId != MENU_POPUPGUTTER &&
				iPartId != MENU_POPUPITEM &&
				iPartId != MENU_POPUPITEM_FOCUSABLE
			);

			RETURN_IF_WIN32_BOOL_FALSE(
				PatBlt(hdc, clipRect.left, clipRect.top, clipRect.right - clipRect.left, clipRect.bottom - clipRect.top, BLACKNESS)
			);
			handled = true;
			return S_OK;
		}
	}

	return E_NOTIMPL;
}

void SharedUxTheme::DoIATHook(PVOID moduleBaseAddress)
{
	if (g_actualDrawThemeBackground)
	{
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "uxtheme.dll", { {"DrawThemeBackground", SharedUxTheme::DrawThemeBackground} });
		Hooking::WriteIAT(moduleBaseAddress, "uxtheme.dll", { {"DrawThemeBackground", SharedUxTheme::DrawThemeBackground} });
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
		{
			{"DrawThemeBackground", SharedUxTheme::DrawThemeBackground}
		});
		Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
		{
			{"DrawThemeBackground", SharedUxTheme::DrawThemeBackground}
		});
	}
}

void SharedUxTheme::UndoIATHook(PVOID moduleBaseAddress)
{
	if (g_actualDrawThemeBackground)
	{
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "uxtheme.dll", { {"DrawThemeBackground", g_actualDrawThemeBackground} });
		Hooking::WriteIAT(moduleBaseAddress, "uxtheme.dll", { {"DrawThemeBackground", g_actualDrawThemeBackground} });
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
		{
			{"DrawThemeBackground", g_actualDrawThemeBackground}
		});
		Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
		{
			{"DrawThemeBackground", g_actualDrawThemeBackground}
		});
	}
}

void SharedUxTheme::DllNotificationCallback(bool load, Hooking::DllNotifyRoutine::DllInfo info)
{
	if (load)
	{
		for (auto moduleName : g_hookModuleList)
		{
			if (!_wcsicmp(moduleName.data(), info.BaseDllName->Buffer))
			{
				DoIATHook(info.DllBase);
			}
		}
	}
}

void SharedUxTheme::Startup() try
{
	if (g_startup)
	{
		return;
	}

	if (TFMain::IsStartAllBackActivated())
	{
		g_disableOnce = true;
	}

	if (g_disableOnce)
	{
		return;
	}

	if (!ThemeHelper::IsThemeAvailable())
	{
		return;
	}

	g_actualDrawThemeBackground = reinterpret_cast<decltype(g_actualDrawThemeBackground)>(DetourFindFunction("uxtheme.dll", "DrawThemeBackground"));
	THROW_LAST_ERROR_IF_NULL(g_actualDrawThemeBackground);

	for (const auto moduleName : g_hookModuleList)
	{
		HMODULE moduleHandle{ GetModuleHandleW(moduleName.data()) };
		if (moduleHandle)
		{
			DoIATHook(moduleHandle);
		}
	}

	Hooking::DllNotifyRoutine::GetInstance().AddCallback(DllNotificationCallback);

	g_startup = true;
}
CATCH_LOG_RETURN()

void SharedUxTheme::Shutdown()
{
	if (!g_startup)
	{
		return;
	}

	if (g_disableOnce)
	{
		return;
	}

	Hooking::DllNotifyRoutine::GetInstance().DeleteCallback(DllNotificationCallback);

	for (const auto moduleName : g_hookModuleList)
	{
		HMODULE moduleHandle{ GetModuleHandleW(moduleName.data()) };
		if (moduleHandle)
		{
			UndoIATHook(moduleHandle);
		}
	}
	for (const auto moduleName : g_delayHookModuleList)
	{
		HMODULE moduleHandle{ GetModuleHandleW(moduleName.data()) };
		if (moduleHandle)
		{
			UndoIATHook(moduleHandle);
		}
	}

	g_startup = false;
}

void SharedUxTheme::DelayStartup()
{
	if (!GetModuleHandleW(L"StartIsBack64.dll") && !GetModuleHandleW(L"StartIsBack32.dll"))
	{
		return;
	}

	if (g_disableOnce)
	{
		return;
	}

	if (!ThemeHelper::IsThemeAvailable())
	{
		return;
	}

	for (const auto moduleName : g_delayHookModuleList)
	{
		HMODULE moduleHandle{ GetModuleHandleW(moduleName.data()) };
		if (moduleHandle)
		{
			DoIATHook(moduleHandle);
		}
	}
}