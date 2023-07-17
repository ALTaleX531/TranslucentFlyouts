#include "pch.h"
#include "Hooking.hpp"
#include "ThemeHelper.hpp"
#include "MenuHandler.hpp"
#include "MenuRendering.hpp"
#include "RegHelper.hpp"
#include "DXHelper.hpp"
#include "ImmersiveContextMenuPatcher.hpp"

namespace TranslucentFlyouts
{
	using namespace std;

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
}

TranslucentFlyouts::ImmersiveContextMenuPatcher::ImmersiveContextMenuPatcher()
{
	try
	{
		m_actualDrawTextW = reinterpret_cast<decltype(m_actualDrawTextW)>(DetourFindFunction("user32.dll", "DrawTextW"));
		THROW_LAST_ERROR_IF_NULL(m_actualDrawTextW);

		m_actualDrawThemeBackground = reinterpret_cast<decltype(m_actualDrawThemeBackground)>(DetourFindFunction("uxtheme.dll", "DrawThemeBackground"));
		THROW_LAST_ERROR_IF_NULL(m_actualDrawThemeBackground);

		m_actualBitBlt = reinterpret_cast<decltype(m_actualBitBlt)>(DetourFindFunction("gdi32.dll", "BitBlt"));
		THROW_LAST_ERROR_IF_NULL(m_actualBitBlt);

		m_actualStretchBlt = reinterpret_cast<decltype(m_actualStretchBlt)>(DetourFindFunction("gdi32.dll", "StretchBlt"));
		THROW_LAST_ERROR_IF_NULL(m_actualStretchBlt);
	}
	catch (...)
	{
		m_internalError = true;
		LOG_CAUGHT_EXCEPTION();
	}
}

TranslucentFlyouts::ImmersiveContextMenuPatcher::~ImmersiveContextMenuPatcher() noexcept
{
	Hooking::DllNotifyRoutine::GetInstance().DeleteCallback(DllNotificationCallback);
	ShutdownHook();
}

TranslucentFlyouts::ImmersiveContextMenuPatcher& TranslucentFlyouts::ImmersiveContextMenuPatcher::GetInstance()
{
	static ImmersiveContextMenuPatcher instance{};
	return instance;
}

HRESULT WINAPI TranslucentFlyouts::ImmersiveContextMenuPatcher::DrawThemeBackground(
	HTHEME  hTheme,
	HDC     hdc,
	int     iPartId,
	int     iStateId,
	LPCRECT pRect,
	LPCRECT pClipRect
)
{
	HRESULT hr{S_OK};

	hr = [&]()
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

		WCHAR themeClassName[MAX_PATH + 1] {};
		RETURN_IF_FAILED(
			ThemeHelper::GetThemeClass(hTheme, themeClassName, MAX_PATH)
		);
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL, !(!_wcsicmp(themeClassName, L"Menu"))
		);

		bool darkMode{ThemeHelper::DetermineThemeMode(hTheme, L"ImmersiveStart", L"Menu", MENU_POPUPBACKGROUND, 0, TMT_FILLCOLOR)};

		MenuHandler::NotifyUxThemeRendering();
		MenuHandler::NotifyMenuDarkMode(darkMode);

		RECT clipRect{*pRect};
		if (pClipRect != nullptr)
		{
			IntersectRect(&clipRect, &clipRect, pClipRect);
		}

		auto& menuRendering{MenuRendering::GetInstance()};
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
				if (SUCCEEDED(menuRendering.DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
				{
					return S_OK;
				}
			}
		}
		// Focusing
		if (iPartId == MENU_POPUPITEMKBFOCUS)
		{
			if (customRendering)
			{
				if (SUCCEEDED(menuRendering.DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
				{
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
					if (SUCCEEDED(menuRendering.DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
					{
						return S_OK;
					}
				}
			}
			if (iStateId == MPI_HOT)
			{
				if (customRendering)
				{
					if (SUCCEEDED(menuRendering.DoCustomThemeRendering(hdc, darkMode, iPartId, iStateId, clipRect, *pRect)))
					{
						return S_OK;
					}
				}

				// System default
				return E_NOTIMPL;
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
		}

		return S_OK;
	}();
	if (FAILED(hr))
	{
		hr = GetInstance().m_actualDrawThemeBackground(
				 hTheme,
				 hdc,
				 iPartId,
				 iStateId,
				 pRect,
				 pClipRect
			 );
	}

	return hr;
}

int WINAPI TranslucentFlyouts::ImmersiveContextMenuPatcher::DrawTextW(
	HDC     hdc,
	LPCWSTR lpchText,
	int     cchText,
	LPRECT  lprc,
	UINT    format
)
{
	HRESULT hr{S_OK};
	int result{0};

	hr = [&]()
	{
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentListviewDC() != hdc &&
			MenuHandler::GetCurrentMenuDC() != hdc
		);
		RETURN_HR_IF(E_INVALIDARG, Utils::IsBadReadPtr(lprc));
		RETURN_HR_IF_EXPECTED(E_NOTIMPL, ((format & DT_CALCRECT) || (format & DT_INTERNAL) || (format & DT_NOCLIP)));
		return ThemeHelper::DrawTextWithAlpha(
				   hdc,
				   lpchText,
				   cchText,
				   lprc,
				   format,
				   result
			   );
	}();
	if (FAILED(hr))
	{
		result = GetInstance().m_actualDrawTextW(hdc, lpchText, cchText, lprc, format);
	}

	return result;
}

BOOL WINAPI TranslucentFlyouts::ImmersiveContextMenuPatcher::BitBlt(
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
	HRESULT hr{S_OK};
	BOOL result{FALSE};

	hr = [&]()
	{
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentMenuDC() != hdc
		);

		result = TRUE;
		return MenuRendering::GetInstance().BltWithAlpha(
			hdc,
			x, y,
			cx, cy,
			hdcSrc,
			x1, y1,
			cx, cy
		);
	}();
	if (FAILED(hr))
	{
		result = GetInstance().m_actualBitBlt(
					 hdc,
					 x, y,
					 cx, cy,
					 hdcSrc,
					 x1, y1,
					 rop
				 );
	}

	return result;
}

BOOL WINAPI TranslucentFlyouts::ImmersiveContextMenuPatcher::StretchBlt(
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
	HRESULT hr{S_OK};
	BOOL result{FALSE};

	hr = [&]()
	{
		RETURN_HR_IF_EXPECTED(
			E_NOTIMPL,
			MenuHandler::GetCurrentMenuDC() != hdcDest
		);

		result = TRUE;
		return MenuRendering::GetInstance().BltWithAlpha(
			hdcDest,
			xDest, yDest,
			wDest, hDest,
			hdcSrc,
			xSrc, ySrc,
			wSrc, hSrc
		);
	}();
	if (FAILED(hr))
	{
		result = GetInstance().m_actualStretchBlt(
					 hdcDest,
					 xDest, yDest,
					 wDest, hDest,
					 hdcSrc,
					 xSrc, ySrc,
					 wSrc, hSrc,
					 rop
				 );
	}

	return result;
}

void TranslucentFlyouts::ImmersiveContextMenuPatcher::DoIATHook(PVOID moduleBaseAddress)
{
	if (m_actualDrawThemeBackground)
	{
		m_hookedFunctions += Hooking::WriteDelayloadIAT(moduleBaseAddress, "uxtheme.dll",
		{
			{"DrawThemeBackground", ImmersiveContextMenuPatcher::DrawThemeBackground}
		});
		m_hookedFunctions += Hooking::WriteIAT(moduleBaseAddress, "uxtheme.dll",
		{
			{"DrawThemeBackground", ImmersiveContextMenuPatcher::DrawThemeBackground}
		});
		m_hookedFunctions += Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
		{
			{"DrawThemeBackground", ImmersiveContextMenuPatcher::DrawThemeBackground}
		});
		m_hookedFunctions += Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
		{
			{"DrawThemeBackground", ImmersiveContextMenuPatcher::DrawThemeBackground}
		});
	}
	if (m_actualDrawTextW)
	{
		m_hookedFunctions += Hooking::WriteDelayloadIAT(moduleBaseAddress, "user32.dll",
		{
			{"DrawTextW", ImmersiveContextMenuPatcher::DrawTextW}
		});
		m_hookedFunctions += Hooking::WriteIAT(moduleBaseAddress, "user32.dll",
		{
			{"DrawTextW", ImmersiveContextMenuPatcher::DrawTextW}
		});
		m_hookedFunctions += Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-ntuser-draw-l1-1-0.dll",
		{
			{"DrawTextW", ImmersiveContextMenuPatcher::DrawTextW}
		});
		m_hookedFunctions += Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-ntuser-draw-l1-1-0.dll",
		{
			{"DrawTextW", ImmersiveContextMenuPatcher::DrawTextW}
		});
	}
	if (m_actualBitBlt && m_actualStretchBlt)
	{
		m_hookedFunctions += Hooking::WriteDelayloadIAT(moduleBaseAddress, "gdi32.dll",
		{
			{"BitBlt", ImmersiveContextMenuPatcher::BitBlt},
			{"StretchBlt", ImmersiveContextMenuPatcher::StretchBlt}
		});
		m_hookedFunctions += Hooking::WriteIAT(moduleBaseAddress, "gdi32.dll",
		{
			{"BitBlt", ImmersiveContextMenuPatcher::BitBlt},
			{"StretchBlt", ImmersiveContextMenuPatcher::StretchBlt}
		});
		m_hookedFunctions += Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-gdi-desktop-l1-1-0.dll",
		{
			{"BitBlt", ImmersiveContextMenuPatcher::BitBlt},
			{"StretchBlt", ImmersiveContextMenuPatcher::StretchBlt}
		});
		m_hookedFunctions += Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-gdi-desktop-l1-1-0.dll",
		{
			{"BitBlt", ImmersiveContextMenuPatcher::BitBlt},
			{"StretchBlt", ImmersiveContextMenuPatcher::StretchBlt}
		});
	}

	if (m_hookedFunctions != 0)
	{
		m_hooked = true;
	}
}

void TranslucentFlyouts::ImmersiveContextMenuPatcher::UndoIATHook(PVOID moduleBaseAddress)
{
	if (m_actualDrawThemeBackground)
	{
		m_hookedFunctions -= Hooking::WriteDelayloadIAT(moduleBaseAddress, "uxtheme.dll",
		{
			{"DrawThemeBackground", m_actualDrawThemeBackground}
		});
		m_hookedFunctions -= Hooking::WriteIAT(moduleBaseAddress, "uxtheme.dll",
		{
			{"DrawThemeBackground", m_actualDrawThemeBackground}
		});
		m_hookedFunctions -= Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
		{
			{"DrawThemeBackground", m_actualDrawThemeBackground}
		});
		m_hookedFunctions -= Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-uxtheme-themes-l1-1-0.dll",
		{
			{"DrawThemeBackground", m_actualDrawThemeBackground}
		});
	}
	if (m_actualDrawThemeBackground)
	{
		m_hookedFunctions -= Hooking::WriteDelayloadIAT(moduleBaseAddress, "user32.dll",
		{
			{"DrawTextW", m_actualDrawTextW}
		});
		m_hookedFunctions -= Hooking::WriteIAT(moduleBaseAddress, "user32.dll",
		{
			{"DrawTextW", m_actualDrawTextW}
		});
		m_hookedFunctions -= Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-ntuser-draw-l1-1-0.dll",
		{
			{"DrawTextW", m_actualDrawTextW}
		});
		m_hookedFunctions -= Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-ntuser-draw-l1-1-0.dll",
		{
			{"DrawTextW", m_actualDrawTextW}
		});
	}
	if (m_actualBitBlt && m_actualStretchBlt)
	{
		m_hookedFunctions -= Hooking::WriteDelayloadIAT(moduleBaseAddress, "gdi32.dll",
		{
			{"BitBlt", m_actualBitBlt},
			{"StretchBlt", m_actualStretchBlt}
		});
		m_hookedFunctions -= Hooking::WriteIAT(moduleBaseAddress, "gdi32.dll",
		{
			{"BitBlt", m_actualBitBlt},
			{"StretchBlt", m_actualStretchBlt}
		});
		m_hookedFunctions -= Hooking::WriteDelayloadIAT(moduleBaseAddress, "ext-ms-win-gdi-desktop-l1-1-0.dll",
		{
			{"BitBlt", m_actualBitBlt},
			{"StretchBlt", m_actualStretchBlt}
		});
		m_hookedFunctions -= Hooking::WriteIAT(moduleBaseAddress, "ext-ms-win-gdi-desktop-l1-1-0.dll",
		{
			{"BitBlt", m_actualBitBlt},
			{"StretchBlt", m_actualStretchBlt}
		});
	}

	if (m_hookedFunctions == 0)
	{
		m_hooked = false;
	}
}

void TranslucentFlyouts::ImmersiveContextMenuPatcher::DllNotificationCallback(bool load, Hooking::DllNotifyRoutine::DllInfo info)
{
	auto& immersiveContextMenuPatcher{GetInstance()};
	if (load)
	{
		for (auto moduleName : g_hookModuleList)
		{
			if (!_wcsicmp(moduleName.data(), info.BaseDllName->Buffer))
			{
				immersiveContextMenuPatcher.DoIATHook(info.DllBase);
			}
		}
	}
}

void TranslucentFlyouts::ImmersiveContextMenuPatcher::StartupHook()
{
	if (m_startup)
	{
		return;
	}
	if (m_internalError)
	{
		return;
	}

	if (m_hooked)
	{
		return;
	}

	for (auto moduleName : g_hookModuleList)
	{
		PVOID dllBaseAddress{reinterpret_cast<PVOID>(GetModuleHandleW(moduleName.data()))};
		if (dllBaseAddress)
		{
			DoIATHook(dllBaseAddress);
		}
	}

	Hooking::DllNotifyRoutine::GetInstance().AddCallback(DllNotificationCallback);

	m_startup = true;
}

void TranslucentFlyouts::ImmersiveContextMenuPatcher::ShutdownHook()
{
	if (!m_startup)
	{
		return;
	}
	if (m_internalError)
	{
		return;
	}

	if (!m_hooked)
	{
		return;
	}

	for (auto moduleName : g_hookModuleList)
	{
		PVOID dllBaseAddress{reinterpret_cast<PVOID>(GetModuleHandleW(moduleName.data()))};
		if (dllBaseAddress)
		{
			UndoIATHook(dllBaseAddress);
		}
	}

	m_startup = false;
}