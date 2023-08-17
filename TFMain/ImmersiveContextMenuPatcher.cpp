#include "pch.h"
#include "Hooking.hpp"
#include "ThemeHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuRendering.hpp"
#include "RegHelper.hpp"
#include "DXHelper.hpp"
#include "SharedUxTheme.hpp"
#include "ImmersiveContextMenuPatcher.hpp"

using namespace std;
using namespace TranslucentFlyouts;

namespace TranslucentFlyouts::ImmersiveContextMenuPatcher
{
	// A list of modules that contain the symbol of C++ class ImmersiveContextMenuHelper,
	// which it means these modules provide methods to create a immersive context menu
	const array g_hookModuleList
	{
		L"explorer.exe"sv,
		L"Narrator.exe"sv,
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
	
	int WINAPI DrawTextW(
		HDC     hdc,
		LPCWSTR lpchText,
		int     cchText,
		LPRECT  lprc,
		UINT    format
	);
	BOOL WINAPI BitBlt(
		HDC   hdc,
		int   x,
		int   y,
		int   cx,
		int   cy,
		HDC   hdcSrc,
		int   x1,
		int   y1,
		DWORD rop
	);
	BOOL WINAPI StretchBlt(
		HDC   hdcDest,
		int   xDest,
		int   yDest,
		int   wDest,
		int   hDest,
		HDC   hdcSrc,
		int   xSrc,
		int   ySrc,
		int   wSrc,
		int   hSrc,
		DWORD rop
	);
	void DllNotificationCallback(bool load, Hooking::DllNotifyRoutine::DllInfo info);

	void DoIATHook(PVOID moduleBaseAddress);
	void UndoIATHook(PVOID moduleBaseAddress);

	decltype(DrawTextW)* g_actualDrawTextW{ nullptr };
	decltype(BitBlt)* g_actualBitBlt{ nullptr };
	decltype(StretchBlt)* g_actualStretchBlt{ nullptr };

	bool g_startup{ false };
}

HRESULT WINAPI ImmersiveContextMenuPatcher::DrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	bool handled{ false };
	HRESULT hr{ S_OK };

	hr = [hTheme, hdc, iPartId, iStateId, pRect, pClipRect, &handled]()
	{
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hTheme);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, hdc);
		RETURN_HR_IF_NULL_EXPECTED(E_INVALIDARG, pRect);
		RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(pRect));
		RETURN_HR_IF_EXPECTED(E_INVALIDARG, IsRectEmpty(pRect) == TRUE);

		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentListviewDC() != hdc &&
			MenuHandler::GetCurrentMenuDC() == nullptr		// To make it compatible with StartIsBack...
		);

		WCHAR themeClassName[MAX_PATH + 1]{};
		RETURN_IF_FAILED(
			ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)
		);

		if (!_wcsicmp(themeClassName, L"Menu"))
		{
			bool darkMode{ ThemeHelper::DetermineThemeMode(hTheme, L"ImmersiveStart", L"Menu", MENU_POPUPBACKGROUND, 0, TMT_FILLCOLOR) };

			MenuHandler::NotifyUxThemeRendering();
			MenuHandler::NotifyMenuDarkMode(darkMode);
			MenuHandler::NotifyMenuStyle(true);

			COLORREF color{ DWMWA_COLOR_NONE };
			if (SUCCEEDED(GetThemeColor(hTheme, MENU_POPUPBORDERS, 0, TMT_FILLCOLORHINT, &color)))
			{
				MenuHandler::NotifyMenuBorderColor(color);
			}

			return SharedUxTheme::DrawThemeBackgroundHelper(
				hTheme,
				hdc,
				iPartId,
				iStateId,
				pRect,
				pClipRect,
				darkMode,
				handled
			);
		}

		return E_NOTIMPL;
	}();
	if (!handled)
	{
		hr = SharedUxTheme::RealDrawThemeBackground(
			hTheme,
			hdc,
			iPartId,
			iStateId,
			pRect,
			pClipRect
		);
	}
	else
	{
		LOG_IF_FAILED(hr);
	}

	return hr;
}

int WINAPI ImmersiveContextMenuPatcher::DrawTextW(
	HDC     hdc,
	LPCWSTR lpchText,
	int     cchText,
	LPRECT  lprc,
	UINT    format
)
{
	bool handled{ false };
	HRESULT hr{S_OK};
	int result{0};

	hr = [hdc, lpchText, cchText, lprc, format, &result, &handled]()
	{
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentListviewDC() != hdc &&
			MenuHandler::GetCurrentMenuDC() != hdc
		);
		RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(lprc));
		RETURN_HR_IF_EXPECTED(E_NOTIMPL, ((format & DT_CALCRECT) || (format & DT_INTERNAL) || (format & DT_NOCLIP)));
		
		handled = true;
		return ThemeHelper::DrawTextWithAlpha(
				   hdc,
				   lpchText,
				   cchText,
				   lprc,
				   format,
				   result
			   );
	}();
	if (!handled)
	{
		result = g_actualDrawTextW(hdc, lpchText, cchText, lprc, format);
	}
	else
	{
		LOG_IF_FAILED(hr);
	}

	return result;
}

BOOL WINAPI ImmersiveContextMenuPatcher::BitBlt(
	HDC   hdc,
	int   x,
	int   y,
	int   cx,
	int   cy,
	HDC   hdcSrc,
	int   x1,
	int   y1,
	DWORD rop
)
{
	bool handled{ false };
	HRESULT hr{S_OK};
	BOOL result{FALSE};

	hr = [hdc, x, y, cx, cy, hdcSrc, x1, y1, rop, &result, &handled]()
	{
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentMenuDC() != hdc
		);

		result = TRUE;
		handled = true;
		return MenuRendering::BltWithAlpha(
				   hdc,
				   x, y,
				   cx, cy,
				   hdcSrc,
				   x1, y1,
				   cx, cy
			   );
	}();
	if (!handled)
	{
		result = g_actualBitBlt(
					 hdc,
					 x, y,
					 cx, cy,
					 hdcSrc,
					 x1, y1,
					 rop
				 );
	}
	else
	{
		LOG_IF_FAILED(hr);
	}

	return result;
}

BOOL WINAPI ImmersiveContextMenuPatcher::StretchBlt(
	HDC   hdcDest,
	int   xDest,
	int   yDest,
	int   wDest,
	int   hDest,
	HDC   hdcSrc,
	int   xSrc,
	int   ySrc,
	int   wSrc,
	int   hSrc,
	DWORD rop
)
{
	bool handled{ false };
	HRESULT hr{S_OK};
	BOOL result{FALSE};

	hr = [hdcDest, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, wSrc, hSrc, rop, &result, &handled]()
	{
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentMenuDC() != hdcDest
		);

		result = TRUE;
		handled = true;
		return MenuRendering::BltWithAlpha(
				   hdcDest,
				   xDest, yDest,
				   wDest, hDest,
				   hdcSrc,
				   xSrc, ySrc,
				   wSrc, hSrc
			   );
	}();
	if (!handled)
	{
		result = g_actualStretchBlt(
					 hdcDest,
					 xDest, yDest,
					 wDest, hDest,
					 hdcSrc,
					 xSrc, ySrc,
					 wSrc, hSrc,
					 rop
				 );
	}
	else
	{
		LOG_IF_FAILED(hr);
	}

	return result;
}

void ImmersiveContextMenuPatcher::DoIATHook(PVOID moduleBaseAddress)
{
	if (g_actualDrawTextW)
	{
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "user32.dll",
		{
			{"DrawTextW", ImmersiveContextMenuPatcher::DrawTextW}
		});
		Hooking::WriteIAT(moduleBaseAddress, "user32.dll",
		{
			{"DrawTextW", ImmersiveContextMenuPatcher::DrawTextW}
		});
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-ntuser-draw-l1-1-0.dll",
		{
			{"DrawTextW", ImmersiveContextMenuPatcher::DrawTextW}
		});
		Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-ntuser-draw-l1-1-0.dll",
		{
			{"DrawTextW", ImmersiveContextMenuPatcher::DrawTextW}
		});
	}
	if (g_actualBitBlt && g_actualStretchBlt)
	{
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "gdi32.dll",
		{
			{"BitBlt", ImmersiveContextMenuPatcher::BitBlt},
			{"StretchBlt", ImmersiveContextMenuPatcher::StretchBlt}
		});
		Hooking::WriteIAT(moduleBaseAddress, "gdi32.dll",
		{
			{"BitBlt", ImmersiveContextMenuPatcher::BitBlt},
			{"StretchBlt", ImmersiveContextMenuPatcher::StretchBlt}
		});
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-gdi-desktop-l1-1-0.dll",
		{
			{"BitBlt", ImmersiveContextMenuPatcher::BitBlt},
			{"StretchBlt", ImmersiveContextMenuPatcher::StretchBlt}
		});
		Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-gdi-desktop-l1-1-0.dll",
		{
			{"BitBlt", ImmersiveContextMenuPatcher::BitBlt},
			{"StretchBlt", ImmersiveContextMenuPatcher::StretchBlt}
		});
	}
}

void ImmersiveContextMenuPatcher::UndoIATHook(PVOID moduleBaseAddress)
{
	if (g_actualDrawTextW)
	{
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "user32.dll",
		{
			{"DrawTextW", g_actualDrawTextW}
		});
		Hooking::WriteIAT(moduleBaseAddress, "user32.dll",
		{
			{"DrawTextW", g_actualDrawTextW}
		});
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-ntuser-draw-l1-1-0.dll",
		{
			{"DrawTextW", g_actualDrawTextW}
		});
		Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-ntuser-draw-l1-1-0.dll",
		{
			{"DrawTextW", g_actualDrawTextW}
		});
	}
	if (g_actualBitBlt && g_actualStretchBlt)
	{
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "gdi32.dll",
		{
			{"BitBlt", g_actualBitBlt},
			{"StretchBlt", g_actualStretchBlt}
		});
		Hooking::WriteIAT(moduleBaseAddress, "gdi32.dll",
		{
			{"BitBlt", g_actualBitBlt},
			{"StretchBlt", g_actualStretchBlt}
		});
		Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-gdi-desktop-l1-1-0.dll",
		{
			{"BitBlt", g_actualBitBlt},
			{"StretchBlt", g_actualStretchBlt}
		});
		Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-gdi-desktop-l1-1-0.dll",
		{
			{"BitBlt", g_actualBitBlt},
			{"StretchBlt", g_actualStretchBlt}
		});
	}
}

void ImmersiveContextMenuPatcher::DllNotificationCallback(bool load, Hooking::DllNotifyRoutine::DllInfo info)
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

void ImmersiveContextMenuPatcher::Startup() try
{
	THROW_HR_IF(E_ILLEGAL_METHOD_CALL, g_startup);

	g_actualDrawTextW = reinterpret_cast<decltype(g_actualDrawTextW)>(DetourFindFunction("user32.dll", "DrawTextW"));
	THROW_LAST_ERROR_IF_NULL(g_actualDrawTextW);

	g_actualBitBlt = reinterpret_cast<decltype(g_actualBitBlt)>(DetourFindFunction("gdi32.dll", "BitBlt"));
	THROW_LAST_ERROR_IF_NULL(g_actualBitBlt);

	g_actualStretchBlt = reinterpret_cast<decltype(g_actualStretchBlt)>(DetourFindFunction("gdi32.dll", "StretchBlt"));
	THROW_LAST_ERROR_IF_NULL(g_actualStretchBlt);

	for (auto moduleName : g_hookModuleList)
	{
		PVOID dllBaseAddress{reinterpret_cast<PVOID>(GetModuleHandleW(moduleName.data()))};
		if (dllBaseAddress)
		{
			DoIATHook(dllBaseAddress);
		}
	}

	Hooking::DllNotifyRoutine::GetInstance().AddCallback(DllNotificationCallback);

	g_startup = true;
}
CATCH_LOG_RETURN()

void ImmersiveContextMenuPatcher::Shutdown() try
{
	THROW_HR_IF(E_ILLEGAL_METHOD_CALL, !g_startup);

	Hooking::DllNotifyRoutine::GetInstance().DeleteCallback(DllNotificationCallback);

	for (auto moduleName : g_hookModuleList)
	{
		PVOID dllBaseAddress{reinterpret_cast<PVOID>(GetModuleHandleW(moduleName.data()))};
		if (dllBaseAddress)
		{
			UndoIATHook(dllBaseAddress);
		}
	}

	g_startup = false;
}
CATCH_LOG_RETURN()