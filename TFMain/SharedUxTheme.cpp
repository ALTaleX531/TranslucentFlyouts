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

	HRESULT WINAPI DrawThemeBackground(
		HTHEME  hTheme,
		HDC     hdc,
		int     iPartId,
		int     iStateId,
		LPCRECT pRect,
		LPCRECT pClipRect
	);

	bool g_startup{ false };
	decltype(DrawThemeBackground)* g_actualDrawThemeBackground{ nullptr };
}

HRESULT WINAPI SharedUxTheme::RealDrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	return g_actualDrawThemeBackground(
		hTheme,
		hdc,
		iPartId,
		iStateId,
		pRect,
		pClipRect
	);
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
				return RealDrawThemeBackground(
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

void SharedUxTheme::Startup() try
{
	THROW_HR_IF(E_ILLEGAL_METHOD_CALL, g_startup);

	g_actualDrawThemeBackground = reinterpret_cast<decltype(g_actualDrawThemeBackground)>(DetourFindFunction("uxtheme.dll", "DrawThemeBackground"));
	THROW_LAST_ERROR_IF_NULL(g_actualDrawThemeBackground);

	for (const auto moduleName : g_hookModuleList)
	{
		HMODULE moduleHandle{ GetModuleHandle(moduleName.data()) };
		if (moduleHandle)
		{
			Hooking::WriteDelayloadIAT(moduleHandle, "uxtheme.dll", { {"DrawThemeBackground", SharedUxTheme::DrawThemeBackground} });
			Hooking::WriteIAT(moduleHandle, "uxtheme.dll", { {"DrawThemeBackground", SharedUxTheme::DrawThemeBackground} });
			Hooking::WriteDelayloadIAT(moduleHandle, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
			{
				{"DrawThemeBackground", SharedUxTheme::DrawThemeBackground}
			});
			Hooking::WriteIAT(moduleHandle, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
			{
				{"DrawThemeBackground", SharedUxTheme::DrawThemeBackground}
			});
		}
	}
	g_startup = true;
}
CATCH_LOG_RETURN()

void SharedUxTheme::Shutdown()
{
	if (!g_startup)
	{
		return;
	}

	for (const auto moduleName : g_hookModuleList)
	{
		HMODULE moduleHandle{ GetModuleHandle(moduleName.data()) };
		if (moduleHandle)
		{
			Hooking::WriteDelayloadIAT(moduleHandle, "uxtheme.dll", { {"DrawThemeBackground", g_actualDrawThemeBackground} });
			Hooking::WriteIAT(moduleHandle, "uxtheme.dll", { {"DrawThemeBackground", g_actualDrawThemeBackground} });
			Hooking::WriteDelayloadIAT(moduleHandle, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
			{
				{"DrawThemeBackground", g_actualDrawThemeBackground}
			});
			Hooking::WriteIAT(moduleHandle, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
			{
				{"DrawThemeBackground", g_actualDrawThemeBackground}
			});
		}
	}
	g_startup = false;
}
